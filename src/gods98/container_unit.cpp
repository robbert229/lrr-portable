// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-002 G98-002: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-002.csv. */
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
#include "view_for_viewports.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "container_native.hpp"
#endif
#include <float.h>
#include <errno.h>
#include "entity.hpp"
#include "game/util.hpp"
#include <limits.h>
#include "record_shared.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
/* C linkage for s_resource_tree_child_iid (IID_IDirect3DRMFrame3,
 * 0049f988), which animclone.c, compiled as C, also reads (finding 616).
 * The header itself is frozen. */
extern "C" {
#include "resource_for_com_object_tree.hpp"
}
#include "scene_resources_for_viewports.hpp"
#include "platform/platform.h"
#include "engine.hpp"
#include "frameloop.hpp"
#include "surface_for_images_unit.hpp"
#include "images.h"
#include "image_adapters.h"
#include "view_for_container_unit.hpp"
#include "crt/crt_for_container_unit.hpp"
#include "scene_pool_for_container_unit.hpp"
#include "scene_resources_for_container_unit.hpp"
#include "surface_for_container_unit.hpp"

typedef uintptr_t (VIEW_COM_CALL *DdiSceneActionQuery)(void*, void*);
typedef uintptr_t (VIEW_COM_CALL *DdiSceneActionCreate)(void*, void*,
                                                        void**);
typedef void (VIEW_COM_CALL *DdiSceneActionRelease)(void*);
typedef void (VIEW_COM_CALL *DdiSceneActionBind)(void*, void*);
typedef void (VIEW_COM_CALL *DdiSceneActionBindOutput)(void*, uint32_t,
                                                       void**);

class SceneObjectTransformInterface
{
public:
    virtual void slot_00(void) = 0; virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0; virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0; virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0; virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0; virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0; virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0; virtual void slot_34(void) = 0;
    virtual void transform_matrix_set(uint32_t mode,
                                      const float* matrix) = 0;
    virtual void slot_3c(uint32_t mode, float x, float y, float z) = 0;
    virtual void frame_scale_set(uint32_t mode, float x, float y,
                                 float z) = 0;
    virtual void rotate_axis(uint32_t mode, float x, float y, float z,
                             float angle) = 0;
};

class SceneObjectCoordinateInterface
{
public:
    DDI_COORD_SLOT(00); DDI_COORD_SLOT(01); DDI_COORD_SLOT(02);
    DDI_COORD_SLOT(03); DDI_COORD_SLOT(04); DDI_COORD_SLOT(05);
    DDI_COORD_SLOT(06); DDI_COORD_SLOT(07); DDI_COORD_SLOT(08);
    DDI_COORD_SLOT(09); DDI_COORD_SLOT(10); DDI_COORD_SLOT(11);
    DDI_COORD_SLOT(12); DDI_COORD_SLOT(13); DDI_COORD_SLOT(14);
    DDI_COORD_SLOT(15); DDI_COORD_SLOT(16); DDI_COORD_SLOT(17);
    DDI_COORD_SLOT(18); DDI_COORD_SLOT(19); DDI_COORD_SLOT(20);
    DDI_COORD_SLOT(21); DDI_COORD_SLOT(22); DDI_COORD_SLOT(23);
    DDI_COORD_SLOT(24); DDI_COORD_SLOT(25); DDI_COORD_SLOT(26);
    DDI_COORD_SLOT(27); DDI_COORD_SLOT(28); DDI_COORD_SLOT(29);
    DDI_COORD_SLOT(30); DDI_COORD_SLOT(31); DDI_COORD_SLOT(32);
    virtual void coordinate_transform_to_output(void* input,
                                               void* output) = 0;
    DDI_COORD_SLOT(34); DDI_COORD_SLOT(35); DDI_COORD_SLOT(36);
    DDI_COORD_SLOT(37); DDI_COORD_SLOT(38); DDI_COORD_SLOT(39);
    DDI_COORD_SLOT(40); DDI_COORD_SLOT(41); DDI_COORD_SLOT(42);
    DDI_COORD_SLOT(43); DDI_COORD_SLOT(44); DDI_COORD_SLOT(45);
    DDI_COORD_SLOT(46); DDI_COORD_SLOT(47); DDI_COORD_SLOT(48);
    DDI_COORD_SLOT(49); DDI_COORD_SLOT(50); DDI_COORD_SLOT(51);
    DDI_COORD_SLOT(52); DDI_COORD_SLOT(53); DDI_COORD_SLOT(54);
    DDI_COORD_SLOT(55); DDI_COORD_SLOT(56); DDI_COORD_SLOT(57);
    DDI_COORD_SLOT(58); DDI_COORD_SLOT(59); DDI_COORD_SLOT(60);
    DDI_COORD_SLOT(61); DDI_COORD_SLOT(62); DDI_COORD_SLOT(63);
    DDI_COORD_SLOT(64); DDI_COORD_SLOT(65);
    virtual void coordinate_transform_in_place(void* coordinates,
                                              void* transform) = 0;
};

class DDiSceneObjectPoseInterface
{
public:
    DDI_POSE_SLOT(00); DDI_POSE_SLOT(04); DDI_POSE_SLOT(08);
    DDI_POSE_SLOT(0c); DDI_POSE_SLOT(10); DDI_POSE_SLOT(14);
    DDI_POSE_SLOT(18); DDI_POSE_SLOT(1c); DDI_POSE_SLOT(20);
    DDI_POSE_SLOT(24); DDI_POSE_SLOT(28); DDI_POSE_SLOT(2c);
    DDI_POSE_SLOT(30); DDI_POSE_SLOT(34); DDI_POSE_SLOT(38);
    DDI_POSE_SLOT(3c); DDI_POSE_SLOT(40); DDI_POSE_SLOT(44);
    DDI_POSE_SLOT(48); DDI_POSE_SLOT(4c); DDI_POSE_SLOT(50);
    DDI_POSE_SLOT(54); DDI_POSE_SLOT(58); DDI_POSE_SLOT(5c);
    virtual void position_get(void* alternate_frame, float position[3]) = 0;
    DDI_POSE_SLOT(64); DDI_POSE_SLOT(68); DDI_POSE_SLOT(6c);
    DDI_POSE_SLOT(70); DDI_POSE_SLOT(74); DDI_POSE_SLOT(78);
    DDI_POSE_SLOT(7c); DDI_POSE_SLOT(80); DDI_POSE_SLOT(84);
    DDI_POSE_SLOT(88); DDI_POSE_SLOT(8c); DDI_POSE_SLOT(90);
    DDI_POSE_SLOT(94); DDI_POSE_SLOT(98); DDI_POSE_SLOT(9c);
    DDI_POSE_SLOT(a0); DDI_POSE_SLOT(a4); DDI_POSE_SLOT(a8);
    DDI_POSE_SLOT(ac); DDI_POSE_SLOT(b0); DDI_POSE_SLOT(b4);
    DDI_POSE_SLOT(b8); DDI_POSE_SLOT(bc); DDI_POSE_SLOT(c0);
    DDI_POSE_SLOT(c4); DDI_POSE_SLOT(c8); DDI_POSE_SLOT(cc);
    DDI_POSE_SLOT(d0); DDI_POSE_SLOT(d4); DDI_POSE_SLOT(d8);
    DDI_POSE_SLOT(dc); DDI_POSE_SLOT(e0); DDI_POSE_SLOT(e4);
    DDI_POSE_SLOT(e8);
    virtual void pose_method_dispatch(void* alternate_frame,
                                     float parameter_3,
                                     float parameter_4,
                                     float parameter_5,
                                     float parameter_6,
                                     float parameter_7,
                                     float parameter_8) = 0;
    virtual void position_set(void* alternate_frame,
                              float x, float y, float z) = 0;
};

class SceneLightInterface
{
public:
    virtual void slot_00(void) = 0; virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0; virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0; virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0; virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0; virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0; virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0; virtual void slot_34(void) = 0;
    virtual int slot_38(uint32_t value) = 0;
    virtual int slot_3c(uint32_t value) = 0;
    virtual int slot_40(uint32_t value) = 0;
};

/* The callback is vtable slot 7 (byte offset 0x1c on x86). Express it as a
 * virtual method so older MSVC compilers use their implicit thiscall ABI. */
class ScenePropertyValueInterface
{
public:
    virtual void* slot_0(void) = 0;
    virtual void* slot_1(void) = 0;
    virtual void* slot_2(void) = 0;
    virtual void* slot_3(void) = 0;
    virtual void* slot_4(void) = 0;
    virtual void* slot_5(void) = 0;
    virtual void* value_record_set(void* value) = 0;
    virtual void* value_record_get(void) = 0;
};

/* Explicit vtable layouts used by the three small resource wrappers below.
 * The corpus uses 32-bit COM-style interfaces; these declarations preserve
 * their method slots without importing a platform SDK. */
class ResourcePropertyInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void* property_get(void) = 0;
};

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeGetNameFn)(
    void*, uint32_t*, char*);

struct SurfacePixelInfo {
    int width;
    int height;
    int reserved2;
    int reserved3;
    int bits_per_pixel;
    int reserved5;
    int row_stride;
    void* pixels;
};

class SurfacePixelInfoOwner {
public:
    virtual void slot00() = 0; virtual void slot04() = 0;
    virtual void slot08() = 0; virtual void slot0c() = 0;
    virtual void slot10() = 0; virtual void slot14() = 0;
    virtual void slot18() = 0; virtual void slot1c() = 0;
    virtual void slot20() = 0; virtual void slot24() = 0;
    virtual void slot28() = 0; virtual void slot2c() = 0;
    virtual void slot30() = 0; virtual void slot34() = 0;
    virtual void slot38() = 0; virtual void slot3c() = 0;
    virtual void slot40() = 0; virtual void slot44() = 0;
    virtual void slot48() = 0; virtual void slot4c() = 0;
    virtual void slot50() = 0; virtual void slot54() = 0;
    virtual void slot58() = 0; virtual void slot5c() = 0;
    virtual SurfacePixelInfo* get_pixel_info() = 0;
};

#if defined(_MSC_VER)
#define TEXTURE_COM_STDCALL __stdcall
#elif defined(__i386__)
#define TEXTURE_COM_STDCALL __attribute__((stdcall))
#else
#define TEXTURE_COM_STDCALL
#endif

typedef uint32_t (TEXTURE_COM_STDCALL *TextureComReleaseFn)(void*);

static void* s_ddiscene_destroyed_list;

/* The 9-slot kind table initialized by FUN_00472710 (orders match the
 * corpus: index 0 "NULL", 1 "MESH", 2 "FRAME", 3 "ANIM", 4 "ACT", then LWS/
 * LWO at 7/8; slots 5/6 stay zero and never match). */
static const char* const s_kinds[9] = {
    "NULL", "MESH", "FRAME", "ANIM", "ACT", NULL, NULL, "LWS", "LWO",
};

#if defined(_MSC_VER) && defined(_M_IX86)
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov edx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 12]
        push edx
        call eax
    }
}
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef void (__attribute__((thiscall)) *DDiScenePayloadDetachMethod)(
    void*, void*);
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    ((DDiScenePayloadDetachMethod)method)(owner, payload);
}
#else
typedef void (*DDiScenePayloadDetachMethod)(void*, void*);
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    ((DDiScenePayloadDetachMethod)method)(owner, payload);
}
#endif

/* Defined below (forward declarations added by tools/relocate_unit.py). */
namespace lego { namespace { void scene_object_allocation_failed(DDiSceneObjectRecord* record); } }
static void ddiscene_destroy_action_frame_records(void* scene_object);
static void ddiscene_destroy_kind1_child_interfaces(void* scene_object);
static void ddiscene_action_channel_apply(void* channel, void* action_record);
static SceneLightInterface* scene_light_interface(void* record);
namespace lego { static void interface_array_forward_sync_slot(void* object, unsigned int byte_offset, void* value); }
namespace lego { static void interface_array_forward_sync_slot80(void* object, uint32_t* count, void** items); }
namespace lego { static int interface_forward_sync_items_reserve(uint32_t count); }
static DDiSceneObjectPoseInterface* ddiscene_pose_interface_resolve( void* object, void* alternate_frame, void** normalized_frame);
static int scene_act_frame_name_match(const char* name);
static void* scene_property_value_record_get(void* property);
static int resource_name_compare_bytes(const char* left, const char* right, int case_sensitive);

namespace lego {
// FUNCTION: LEGORR 0x00472800
void ddiscene_object_pool_shutdown(void)
{
    unsigned char bank = 0;
    uint32_t i;
    DDiSceneObjectRecord** block_slot = s_scene_object_blocks;
    /* Scan all twenty banks as retail does. Clear the reconstruction's
     * owner slots too, so failed application init can unwind exactly once. */
    do {
        DDiSceneObjectRecord* block = *block_slot;
        if (block != 0) {
            const uint32_t count = 1u << (bank & 0x1fu);
            for (i = 0; i < count; ++i) {
                DDiSceneObjectRecord* record = block + i;
                if (record->next == record) {
                    if (s_object_destroy != 0)
                        s_object_destroy(record, 1);
                    else
                        scene_object_partial_release(record);
                }
            }
            engine_heap_free(block);
            *block_slot = 0;
        }
        ++block_slot;
        ++bank;
    } while (block_slot < &s_scene_object_blocks[DDI_SCENE_POOL_BANKS]);
    s_scene_object_free_head = 0;
    s_scene_object_level_count = 0;
    for (i = 0; i < s_auxiliary_count; ++i) {
        if (s_auxiliary_allocations != 0 && s_auxiliary_allocations[i] != 0) {
            if (s_auxiliary_release != 0)
                s_auxiliary_release(s_auxiliary_allocations[i]);
            else
                engine_heap_free(s_auxiliary_allocations[i]);
        }
    }
    if (s_shared_directory != 0) {
        if (s_auxiliary_release != 0)
            s_auxiliary_release(s_shared_directory);
        else
            engine_heap_free(s_shared_directory);
    }
    s_auxiliary_allocations = 0;
    s_auxiliary_count = 0;
    s_shared_directory = 0;
}
} // namespace lego

// FUNCTION: LEGORR 0x004728c0
void shared_texture_directory_set(const char* path)
{
    s_shared_texture_dir = string_duplicate(path);
}

// FUNCTION: LEGORR 0x004728e0
void renderer_flag_bit6_set(int value)
{
    s_renderer_flag_word = (value != 0) ? (s_renderer_flag_word | 0x40u)
                                        : (s_renderer_flag_word & ~0x40u);
}

// FUNCTION: LEGORR 0x00472900
void scene_end_hook_register(void* hook, void* value)
{
    s_scene_end_hook = hook;
    s_scene_end_value = value;
}

// FUNCTION: LEGORR 0x00472920
void scene_sample_hook_register(void* hook, void* value)
{
    s_scene_sample_hook = hook;
    s_scene_sample_value = value;
    s_renderer_flag_word |= 0x40u;
}

/* SceneRootObjectGet (FUN_00472940) — the corpus's root-object read. */
// FUNCTION: LEGORR 0x00472940
void* scene_root_object_get(void)
{
    return s_scene_root;
}

namespace lego {
// FUNCTION: LEGORR 0x00472950
void* ddiscene_object_allocate(const DDiSceneObjectDescriptor* descriptor)
{
    DDiSceneObjectRecord* record;
    unsigned int i;
    if (s_scene_object_free_head == 0) ddiscene_object_array_grow();
    record = s_scene_object_free_head;
    if (record == 0) return 0;
    s_scene_object_free_head = record->next;
    memset(record, 0, sizeof(*record));
    record->next = record;
    if (s_channel_create == 0) {
        scene_object_return_to_pool(record);
        return 0;
    }
    for (i = 0; i < 3; ++i) {
        /* CreateFrame(parent[0]), CreateFrame(new[0]),
         * CreateFrame(parent[2]): FUN_00472950 does not copy the parent's
         * kind/flags and does not use parent[1] for its middle frame. */
        void* parent = i == 1u ? record->channels[0] :
            (descriptor != 0 ? descriptor->channels[i] : 0);
        if (s_channel_create(parent, &record->channels[i]) != 0) {
            scene_object_allocation_failed(record);
            return 0;
        }
        if (record->channels[i] == 0) {
            scene_object_allocation_failed(record);
            return 0;
        }
    }
    /* The retail call stores the owning scene record in the hook property's
     * 32-bit callback word.  Keep that pointer-to-word conversion on the
     * original 32-bit ABI; a wider host cannot represent it in that field. */
    if (sizeof(void*) == 4u) {
        const int owner_record = (int)(long)record;
        for (i = 0; i < 3; ++i) {
            ddiscene_animation_hook_create(
                record->channels[i], owner_record, 0, 0, 0, 0, 0, 0, 0, 0);
            if (ddiscene_animation_hook_record_get(record->channels[i]) == 0) {
                scene_object_allocation_failed(record);
                return 0;
            }
        }
    }
    return record;
}
} // namespace lego

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00472a40
extern "C" void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object)
{
    ddiscene_object_destroy_with_mode(scene_object, 0);
}
} // namespace unported_ledger_stubs

/* DDiSceneObjectDestroy (FUN_00472a50): use the renderer's full destructor
 * when installed; the local fallback handles the recovered state and owned
 * payload releases that do not need renderer traversal. */
// FUNCTION: LEGORR 0x00472a50
void ddiscene_object_destroy_with_mode(void* scene_object, int mode)
{
    uint32_t kind;
    if (scene_object == NULL) return;
    if (s_ddiscene_object_destroy_hook != NULL) {
        s_ddiscene_object_destroy_hook(scene_object, mode,
                                       s_ddiscene_object_destroy_context);
        return;
    }

    memcpy(&kind, (char*)scene_object + 0x10, sizeof(kind));
    if (kind != 6u) {
        ddiscene_scene_object_attach_or_unlink(scene_object, NULL);
    }

    /* FUN_00472a50's mode-zero path for kinds 3 and 4 clears the linked
     * scene record's +0x10 word and returns before the rest of destruction.
     * The renderer-specific detach/release sequence remains owned by the
     * installed hook, but this state transition is represented by the scene
     * record layout already used by the animation helpers. */
    if ((kind == 3u || kind == 4u) && mode == 0) {
        void* linked_record;
        memcpy(&linked_record, (char*)scene_object + 0x24,
               sizeof(linked_record));
        if (linked_record != NULL) {
            const uint32_t inactive = 0u;
            memcpy((char*)linked_record + 0x10, &inactive,
                   sizeof(inactive));
            return;
        }
    }

    if (kind == 3u) {
        void* scene_interface;
        void* owner_link;
        memcpy(&scene_interface, (char*)scene_object + 4,
               sizeof(scene_interface));
        owner_link = scene_interface != NULL
            ? ddiscene_action_record_field_1c_pointer_get(scene_interface)
            : NULL;
        if (owner_link != NULL) {
            scene_object_owner_link_release(
                (SceneObjectOwnerLinkRecord*)owner_link);
        }
    }

    if (kind == 1u)
        ddiscene_destroy_kind1_child_interfaces(scene_object);

    /* Kind three releases its retained owner-link record above; kind four
     * releases each action-frame property's owned buffer and attached data. */
    if (kind == 4u) ddiscene_destroy_action_frame_records(scene_object);

    /* FUN_00472a50 calls FUN_00475c80 on the non-early-return path. That
     * helper owns the kind-specific payload release and clears the +0x0c
     * field even when the external scene manager is absent. */
    ddiscene_payload_release(scene_object);
    if (lego::ddiscene_object_pool_owns(scene_object)) {
        lego::ddiscene_object_record_release(scene_object);
        return;
    }
    memcpy((char*)scene_object + 0x28, &s_ddiscene_destroyed_list,
           sizeof(s_ddiscene_destroyed_list));
    s_ddiscene_destroyed_list = scene_object;
}

// FUNCTION: LEGORR 0x00472cd0
EntityAssembly* entity_assembly_parse(BlockFile* bf, const char* root,
                                     uint32_t feature)
{
    if (bf == NULL || root == NULL || *root == '\0') {
        return NULL;
    }

    char path[512]; /* longest query: root::<name>::KEY */
    snprintf(path, sizeof path, "%s::Activities", root);
    BlockRec* acts = block_subgroup(bf, path);
    if (acts == NULL) {
        /* corpus: no Activities subgroup => no assembly record */
        return NULL;
    }

    EntityAssembly* ea = (EntityAssembly*)calloc(1, sizeof *ea);
    if (ea == NULL) {
        return NULL;
    }

    /* Root SCALE float (read <root>::Scale). The corpus applies a nonzero
     * scale through the record vtable; the value is kept for the caller. */
    snprintf(path, sizeof path, "%s::SCALE", root);
    ea->scale = block_get_float(bf, path, 0.0f);

    size_t cap = 0;
    for (BlockRec* r = acts; r != NULL;
         r = block_next_sibling(r)) {
        const char* key = block_name(r);   /* "Activity_<State>", '!' possible */
        const char* grp = block_value(r);  /* target animation group */
        if (key == NULL || grp == NULL) {
            continue;
        }
        int was_flagged = 0;
        if (key[0] == '!') {
            if (feature & ENTITY_FEATURE_REDUCE_ANIM) {
                /* reduce-animation: activity omitted entirely */
                continue;
            }
            was_flagged = 1;
            ++key; /* strip '!' — processed under the default feature */
        }

        snprintf(path, sizeof path, "%s::%s::FILE", root, grp);
        const char* file = block_get_strptr(bf, path);
        if (file == NULL) {
            /* no FILE => the corpus makes no scene for this activity */
            continue;
        }

        if (ea->count >= cap) {
            size_t ncap = cap ? cap * 2 : 8;
            EntityActivity* na = (EntityActivity*)realloc(ea->acts,
                                                          ncap * sizeof *na);
            if (na == NULL) {
                break;
            }
            ea->acts = na;
            cap = ncap;
        }

        EntityActivity* a = &ea->acts[ea->count++];
        memset(a, 0, sizeof *a);
        snprintf(a->state, sizeof a->state, "%s", key);
        snprintf(a->name, sizeof a->name, "%s", grp);
        snprintf(a->file, sizeof a->file, "%s", file);
        a->reduce_anim = was_flagged;

        snprintf(path, sizeof path, "%s::%s::TRANSCOEF", root, grp);
        a->transcoef = block_get_float(bf, path, 0.0f);
        snprintf(path, sizeof path, "%s::%s::TRIGGER", root, grp);
        a->trigger = block_get_int(bf, path, 0);
        snprintf(path, sizeof path, "%s::%s::LWSFILE", root, grp);
        a->lwsfile = block_get_bool_path(bf, path);
        snprintf(path, sizeof path, "%s::%s::LOOPING", root, grp);
        a->looping = block_get_bool_path(bf, path);
        snprintf(path, sizeof path, "%s::%s::SAMPLE", root, grp);
        const char* sample = block_get_strptr(bf, path);
        if (sample != NULL) {
            snprintf(a->sample, sizeof a->sample, "%s", sample);
        }
    }
    return ea;
}

namespace unported_ledger_stubs {
// SceneActionNamePresent
// FUNCTION: LEGORR 0x00473340
uint32_t ledger_stub_00473340_SceneActionNamePresent(
    void* scene_object, void* action_name)
{
    return scene_action_name_present(scene_object, action_name);
}
} // namespace unported_ledger_stubs

/* SceneActionNameSet @ 0x473370. Action lookup is provided by the renderer's
 * action-record adapters; action-channel application and the scene-owned
 * name/flag/callback writes use the retail record layout directly. */
// FUNCTION: LEGORR 0x00473370
uint32_t scene_action_name_set(void* scene_object,
                               const char* action_name)
{
    char* previous_name = NULL;
    void* requested_record;
    uint32_t result = 0u;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 4u) return 0u;

    requested_record = ddiscene_action_name_record_find(
        scene_object, action_name, 1u);
    if (requested_record != NULL) {
        void* slot_storage;
        memcpy(&slot_storage, (char*)scene_object + 0x0c,
               sizeof(slot_storage));
        if (slot_storage != NULL) {
            memcpy(&previous_name, slot_storage, sizeof(previous_name));
            if (previous_name != NULL) {
                void* previous_record = ddiscene_action_name_record_find(
                    scene_object, previous_name, 0u);
                if (previous_record != NULL) {
                    void* secondary;
                    memcpy(&secondary, (char*)scene_object + 8,
                           sizeof(secondary));
                    ddiscene_action_channel_apply(secondary,
                                                  previous_record);
                }
            }
        }
        {
            void* primary;
            memcpy(&primary, (char*)scene_object + 4, sizeof(primary));
            ddiscene_action_channel_apply(primary, requested_record);
        }
        if (action_name != NULL) {
            const size_t length = strlen(action_name);
            char* name_copy = (char*)engine_heap_alloc((long)(length + 1u));
            if (name_copy != NULL) {
                memcpy(name_copy, action_name, length + 1u);
                ddiscene_animation_callbacks_set(scene_object, name_copy,
                                                 NULL, NULL, NULL);
                *(uint32_t*)((char*)scene_object + 0x14) |= 2u;
                result = 1u;
            }
        }
        if (previous_name != NULL) engine_heap_free(previous_name);
    }
    {
        typedef void (*SceneActionChangedCallback)(void*, void*);
        SceneActionChangedCallback callback = NULL;
        void* callback_argument;
        memcpy(&callback, (char*)scene_object + 0x18, sizeof(callback));
        memcpy(&callback_argument, (char*)scene_object + 0x1c,
               sizeof(callback_argument));
        if (callback != NULL) callback(scene_object, callback_argument);
    }
    return result;
}

// FUNCTION: LEGORR 0x00473460
int light_interface_call_slot_40(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_40(value) == 0;
}
// FUNCTION: LEGORR 0x00473480
int light_interface_call_slot_3c(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_3c(value) == 0;
}
/* Light interface forwarding leaves (Finding 430). The reference treats
 * zero from each vtable call as success. */
// FUNCTION: LEGORR 0x004734a0
int light_interface_call_slot_38(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_38(value) == 0;
}

// FUNCTION: LEGORR 0x004734c0
void advisor_companion_transform_apply(void* unused, void** object_reference)
{
    void* object;
    (void)unused;
    if (object_reference == NULL) {
        return;
    }
    object = *object_reference;
    if (s_advisor_object_apply != NULL) {
        s_advisor_object_apply(object, s_advisor_companion_transform,
                               &object_reference);
    }
    if (s_advisor_scene_apply != NULL) {
        s_advisor_scene_apply(object, s_advisor_companion_transform,
                              &object_reference);
    }
}

namespace unported_ledger_stubs {
// DDiSceneLightCreate
// FUNCTION: LEGORR 0x004734f0
void* ledger_stub_004734f0_DDiSceneLightCreate(
    void* owner, uint32_t value_a, uint32_t value_b,
    uint32_t value_c, uint32_t value_d)
{
    return ddiscene_light_create(owner, value_a, value_b, value_c, value_d);
}

// DDiSceneObjectKindModeSetup
// FUNCTION: LEGORR 0x00473560
void* ledger_stub_00473560_DDiSceneObjectKindModeSetup(void* owner, int mode)
{
    return ddiscene_object_kind_mode_setup(owner, mode);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00473680
uint32_t scene_object_wrapped_pointer_get(const void* wrapped)
{
    uint32_t value;
    __builtin_memcpy(&value, wrapped, sizeof(value));
    return value;
}

// FUNCTION: LEGORR 0x00473690
/* EntityVisualResourceLoad (FUN_00473690): resolve the resource key, reuse
 * an existing cache entry where available, otherwise create one, then run
 * common preparation and the kind-four act-frame build before attachment. */
void* entity_visual_resource_load(void* scene_object)
{
    EntityVisualResourceLoadHooks* hooks =
        &s_entity_visual_resource_load_hooks;
    int kind;
    const char* name;
    void* resource = NULL;
    if (scene_object == NULL || hooks->kind_get == NULL ||
        hooks->name_get == NULL) return NULL;
    kind = hooks->kind_get(scene_object, hooks->context);
    name = hooks->name_get(scene_object, hooks->context);
    if (name != NULL && hooks->resource_lookup != NULL)
        resource = hooks->resource_lookup(name, hooks->context);
    if (resource == NULL && hooks->resource_create != NULL)
        resource = hooks->resource_create(scene_object, name, hooks->context);
    if (resource == NULL) return NULL;
    if (hooks->resource_prepare != NULL)
        hooks->resource_prepare(resource, scene_object, hooks->context);
    if (kind == 4 && hooks->animation_build != NULL)
        hooks->animation_build(resource, scene_object, hooks->context);
    if (hooks->resource_attach != NULL)
        hooks->resource_attach(scene_object, resource, hooks->context);
    return resource;
}

/* EntityFlag80Set (FUN_00473b20) — see record.hpp. */
// FUNCTION: LEGORR 0x00473b20
void entity_flag_80_set_corpus(void* record, int set)
{
    uint32_t* word = (uint32_t*)((char*)record + 0x14);
    if (set != 0) {
        *word |= 0x80u;
    }
    else {
        *word &= ~0x80u;
    }
}

/* SceneRenderLockToggle (FUN_00473b40): update the paired render interfaces
 * on the transition into or out of lock mode, then toggle flag bit 3. The
 * channel slots use the corpus offsets; rec_ptr preserves this port's
 * native-width pointer-slot convention. */
// FUNCTION: LEGORR 0x00473B40
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_render_lock_toggle(void* scene_object,
                                                int enabled)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [esi + 14h]
        and eax, 8
        test ecx, ecx
        je scene_render_lock_toggle_disable_check
        test eax, eax
        jne scene_render_lock_toggle_disable_gate
        mov eax, dword ptr [esi + 4]
        mov ecx, dword ptr [esi + 8]
        push eax
        push ecx
        call render_interface_pair_update
        mov eax, dword ptr [esi + 14h]
        add esp, 8
        or al, 8
        mov dword ptr [esi + 14h], eax
        pop esi
        ret
    scene_render_lock_toggle_disable_gate:
        test ecx, ecx
        jne scene_render_lock_toggle_done
    scene_render_lock_toggle_disable_check:
        test eax, eax
        je scene_render_lock_toggle_done
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi]
        push edx
        push eax
        call render_interface_pair_update
        mov eax, dword ptr [esi + 14h]
        add esp, 8
        and al, 0f7h
        mov dword ptr [esi + 14h], eax
    scene_render_lock_toggle_done:
        pop esi
        ret
    }
}
#else
void scene_render_lock_toggle(void* scene_object, int enabled)
{
    uint32_t flags;
    if (scene_object == NULL) return;
    flags = rec_dword(scene_object, 0x14);
    if (enabled != 0) {
        if ((flags & 8u) == 0u) {
            render_interface_pair_update(rec_ptr(scene_object, 0x08u),
                                         rec_ptr(scene_object, 0x04u));
            rec_dword_put(scene_object, 0x14, flags | 8u);
        }
        return;
    }
    if ((flags & 8u) != 0u) {
        render_interface_pair_update(rec_ptr(scene_object, 0x00u),
                                     rec_ptr(scene_object, 0x04u));
        rec_dword_put(scene_object, 0x14, flags & ~8u);
    }
}
#endif
/* The mapped SceneObjectFlagBit3Get body follows the corpus's unchecked
 * +0x14 load; animation_object_visibility_get retains its null-safe path. */
// FUNCTION: LEGORR 0x00473ba0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t scene_object_flag_bit3_get_target(
    const void* scene_object)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 14h]
        and eax, 0ffh
        shr eax, 3
        and eax, 1
        ret
    }
}
#else
uint32_t scene_object_flag_bit3_get_target(const void* scene_object)
{
    return (*(const uint32_t*)((const char*)scene_object + 0x14) & 0xffu)
           >> 3 & 1u;
}
#endif

/* ResourceNameRegisterOrQuery (FUN_00473bc0) builds the seven-word callback
 * record, walks the interface root, and resolves the selected object through
 * SceneObjectOwnerLinkEnsure. The portable ResourceTreeNode helper below is
 * kept for host-side callers; this is the retail pointer-to-root ABI. */
// FUNCTION: LEGORR 0x00473bc0
void* resource_name_register_or_query_target(
    void** root, const char* name, int mode,
    uint32_t* match_count_or_index)
{
    const char* cursor = name;
    uint32_t remaining = 0xffffffffu;
    ResourceNameMatchCallbackContext query;

    do {
        char value;
        if (remaining == 0u) break;
        --remaining;
        value = *cursor;
        ++cursor;
        if (value == '\0') break;
    } while (1);

    query.name = name;
    query.name_length = ~remaining - 1u;
    query.case_sensitive = 0u;
    query.matched_object = NULL;
    query.match_count = 0u;
    query.mode = mode;
    query.selected_index = match_count_or_index != NULL
        ? *match_count_or_index : 0u;

    if (root != NULL && *root != NULL) {
        (void)resource_tree_callback_walk_target(*root, 0,
            resource_name_match_callback_target, &query);
    }

    if (mode == 0 || mode == 2) {
        return query.matched_object != NULL
            ? scene_object_owner_link_ensure(query.matched_object) : NULL;
    }
    if (mode == 1) {
        if (match_count_or_index != NULL)
            *match_count_or_index = query.match_count;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00473c60
const char* xf_texture_name_format(void* texture_record,
                                   const char* name_base,
                                   const int* frame_index)
{
    static char output[1024];
    char path[1024];
    const char* frame_name = NULL;
    const char* base_name = name_base != NULL ? name_base : "";
    void* frame_record = NULL;
    const char* path_name;
    uint32_t kind = 0u;
    void* action_name_list = NULL;
    const char* action_name = NULL;
    void* action_state = NULL;
    void* resolved_object = NULL;
    int written = 0;

    output[0] = '\0';
    if (texture_record == NULL) return output;
    memcpy(&kind, (char*)texture_record + 0x10, sizeof(kind));
    if (kind == 4u) {
        memcpy(&action_name_list, (char*)texture_record + 0x0c,
               sizeof(action_name_list));
        if (action_name_list != NULL)
            memcpy(&action_name, action_name_list, sizeof(action_name));
        if (action_name != NULL)
            frame_record = ddiscene_frame_record_find(
                texture_record, action_name, 0);
    }
    else if (kind == 3u) {
        memcpy(&frame_record, (char*)texture_record + 4,
               sizeof(frame_record));
    }

    action_state = frame_record != NULL
        ? ddiscene_action_record_field_1c_pointer_get(frame_record) : NULL;
    if (action_state != NULL)
        memcpy(&resolved_object, (char*)action_state + 0x0c,
               sizeof(resolved_object));

    if (resolved_object == NULL) {
        path_name = frame_record != NULL
            ? scene_property_secondary_name_get(frame_record) : NULL;
        if (path_name != NULL) {
            size_t length = strlen(path_name);
            if (length >= sizeof(path)) length = sizeof(path) - 1u;
            memcpy(path, path_name, length);
            path[length] = '\0';
            crt_string_to_lower(path);
            frame_name = path;
            {
                const char* scan = path;
                while (*scan != '\0') {
                    if (*scan == '\\') frame_name = scan + 1;
                    ++scan;
                }
            }
        }
        if (frame_index != NULL) {
#if defined(_MSC_VER)
            written = _snprintf(output, sizeof(output) - 1u,
                "xf_????????_%s_%s_%0.2d_DDc_00", base_name,
                frame_name != NULL ? frame_name : "", *frame_index);
#else
            written = snprintf(output, sizeof(output) - 1u,
                "xf_????????_%s_%s_%0.2d_DDc_00", base_name,
                frame_name != NULL ? frame_name : "", *frame_index);
#endif
        }
        else {
#if defined(_MSC_VER)
            written = _snprintf(output, sizeof(output) - 1u,
                "xf_????????_%s_%s_??_DDc_00", base_name,
                frame_name != NULL ? frame_name : "");
#else
            written = snprintf(output, sizeof(output) - 1u,
                "xf_????????_%s_%s_??_DDc_00", base_name,
                frame_name != NULL ? frame_name : "");
#endif
        }
    }
    else if (frame_index != NULL) {
#if defined(_MSC_VER)
        written = _snprintf(output, sizeof(output) - 1u, "%s_%0.2i",
                            base_name, *frame_index);
#else
        written = snprintf(output, sizeof(output) - 1u, "%s_%0.2i",
                           base_name, *frame_index);
#endif
    }
    else {
#if defined(_MSC_VER)
        written = _snprintf(output, sizeof(output) - 1u, "%s_??", base_name);
#else
        written = snprintf(output, sizeof(output) - 1u, "%s_??", base_name);
#endif
    }
    (void)written;
    output[sizeof(output) - 1u] = '\0';
    return output;
}

// FUNCTION: LEGORR 0x00473da0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_object_owner_set(void* scene_object,
                                              uint32_t owner)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        mov dword ptr [ecx + 20h], eax
        ret
    }
}
#else
void scene_object_owner_set(void* scene_object, uint32_t owner)
{
    if (scene_object == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)scene_object + 0x20, &owner, sizeof(owner));
}
#endif
// FUNCTION: LEGORR 0x00473db0
uint32_t scene_object_owner_get(const void* scene_object)
{
    uint32_t owner;
    __builtin_memcpy(&owner, (const char*)scene_object + 0x20,
                     sizeof(owner));
    return owner;
}

// FUNCTION: LEGORR 0x00473dc0
void renderer_fog_enable_and_color_apply(int enabled)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_enable(enabled);
    scene->fog_color_enable(enabled != 0 ? s_scene_fog_color : 0u);
}

// FUNCTION: LEGORR 0x00473e10
void renderer_fog_color_set(float red, float green, float blue)
{
    (void)green; /* reference code converts the first argument twice */
    const unsigned int first = (unsigned int)float_to_long(red * 255.0f);
    const unsigned int second = (unsigned int)float_to_long(red * 255.0f);
    const unsigned int third = (unsigned int)float_to_long(blue * 255.0f);
    const unsigned int packed =
        ((first | ((second | 0xffffff00u) << 8)) << 8) | third;
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    s_scene_fog_color = packed;
    scene->fog_color_set(packed);
}
// FUNCTION: LEGORR 0x00473e70
void renderer_fog_mode_set(unsigned int mode)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_mode_set(mode);
    scene->fog_render_mode_set(s_scene_fog_mode);
}

// FUNCTION: LEGORR 0x00473ea0
void renderer_fog_parameters_set(unsigned int start, unsigned int end,
                                 unsigned int density)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_parameters_set(start, end, density);
}

/* RenderObjectStateApplyToEntries (FUN_00473ec0): state kind 4 snapshots
 * the child set and applies each child callback; kind 1 applies the surface
 * property to each layer in order. */
// FUNCTION: LEGORR 0x00473ec0
void render_object_state_apply_to_entries(void* object, uint32_t value,
                                          const RenderObjectStateOps* ops)
{
    unsigned int i;
    int kind;
    if (object == NULL || ops == NULL || ops->object_kind == NULL) return;
    kind = ops->object_kind(ops->context, object);
    if (kind == 4 && ops->children_snapshot != NULL &&
        ops->child_state_apply != NULL) {
        void** children = NULL;
        unsigned int count = 0;
        if (ops->children_snapshot(ops->context, object, &children,
                                   &count) == 0 || children == NULL) {
            return;
        }
        for (i = 0; i < count; ++i) {
            if (children[i] != NULL)
                ops->child_state_apply(ops->context, children[i], value);
        }
        if (ops->children_snapshot_release != NULL)
            ops->children_snapshot_release(ops->context, children);
    }
    else if (kind == 1 && ops->layer_count != NULL &&
             ops->layer_property_set != NULL) {
        unsigned int count = ops->layer_count(ops->context, object);
        for (i = 0; i < count; ++i) {
            ops->layer_property_set(ops->context, object, i, value);
        }
    }
}

// FUNCTION: LEGORR 0x00474410
int parse_indexed_surface_name(const char* path, uint32_t* index_out)
{
    const char* component;
    const char* digits;
    uint32_t value;
    unsigned int i;
    if (path == NULL || index_out == NULL) return 0;
    component = path;
    while (*path != '\0') {
        if (*path == '\\' || *path == '/') component = path + 1;
        ++path;
    }
    if (component[0] != 'a' && component[0] != 'A') return 0;
    digits = component + 1;
    value = 0;
    for (i = 0; i < 3; ++i) {
        if (digits[i] < '0' || digits[i] > '9') return 0;
        value = value * 10u + (uint32_t)(digits[i] - '0');
    }
    if (digits[3] != '_') return 0;
    *index_out = value;
    return 1;
}

/* TextureSurfaceCreate (FUN_004744f0): mode zero resolves the configured
 * renderer surface database; nonzero mode uses the named bitmap path. The
 * configured database and its COM object tree are renderer-owned, so the
 * hook handles that route. Mode one can use the portable BMP decoder and its
 * release-tracked host record. */
// FUNCTION: LEGORR 0x004744f0
void* texture_surface_create(const char* name, uint32_t mode,
                             uint32_t* value_out, uint32_t mode_data)
{
    if (name == NULL) return NULL;
    if (s_texture_surface_create_hook != NULL) {
        return s_texture_surface_create_hook(s_texture_surface_create_context,
                                             name, mode, value_out,
                                             mode_data);
    }
    if (mode != 1u) return NULL;
    return texture_surface_create_host_record(name, mode);
}

/* Retail TextureRelease releases both COM surfaces in its two-pointer
 * record. Each reference is released repeatedly until the COM object's
 * Release method reports that its count reached zero. This record is not a
 * TextureSurface: that port type owns CPU pixels and has a separate cache
 * reference policy above. */
// FUNCTION: LEGORR 0x00474710
void texture_release_record(TextureReleaseRecord* record)
{
    void* interface_object;
    void** interface_vtable;
    TextureComReleaseFn release;
    uint32_t reference_count;

    if (record == NULL) return;
    if (texture_surface_create_host_record_release(record)) return;

    interface_object = record->surface;
    if (interface_object != NULL) {
        interface_vtable = *(void***)interface_object;
        release = (TextureComReleaseFn)interface_vtable[2];
        reference_count = release(interface_object);
        while (reference_count != 0u) {
            interface_object = record->surface;
            interface_vtable = *(void***)interface_object;
            release = (TextureComReleaseFn)interface_vtable[2];
            reference_count = release(interface_object);
        }
    }

    interface_object = record->texture;
    if (interface_object != NULL) {
        interface_vtable = *(void***)interface_object;
        release = (TextureComReleaseFn)interface_vtable[2];
        reference_count = release(interface_object);
        while (reference_count != 0u) {
            interface_object = record->texture;
            interface_vtable = *(void***)interface_object;
            release = (TextureComReleaseFn)interface_vtable[2];
            reference_count = release(interface_object);
        }
    }
}

namespace lego {
// FUNCTION: LEGORR 0x00474760
void interface_array_forward_sync(void* owner_link, void* source_record,
                                  void* state_value)
{
    unsigned char* record = (unsigned char*)owner_link;
    unsigned char* source = (unsigned char*)source_record;
    uint32_t kind;
    uint32_t count;
    uint32_t i;
    void* primary;
    void* primary_a;
    void* primary_b;
    void* secondary;
    void* secondary_child;
    uint32_t flags;
    void* selector;

    if (record == NULL) return;
    if (s_interface_array_forward_sync_hooks.record_read != NULL) {
        if (!s_interface_array_forward_sync_hooks.record_read(
                s_interface_array_forward_sync_hooks.context, owner_link,
                &primary_a, &primary_b, &secondary, &secondary_child,
                &kind, &flags)) return;
        primary = kind == 6u ? primary_a : primary_b;
    }
    else {
        /* The raw owner-link fields are four-byte pointers in the reference
         * ABI. Wider hosts must bind the typed record accessor above. */
        if (sizeof(void*) != 4u) return;
        memcpy(&kind, record + 0x10, sizeof(kind));
        memcpy(&primary_a, record + 0x00, sizeof(primary_a));
        memcpy(&primary_b, record + 0x04, sizeof(primary_b));
        primary = kind == 6u ? primary_a : primary_b;
        memcpy(&secondary, record + 0x08, sizeof(secondary));
        memcpy(&flags, record + 0x14, sizeof(flags));
        secondary_child = NULL;
        if (secondary != NULL)
            memcpy(&secondary_child, (unsigned char*)secondary + 0x08,
                   sizeof(secondary_child));
    }
    if (primary == NULL) return;

    if (state_value != NULL) {
        count = (uint32_t)(uintptr_t)state_value;
        interface_array_forward_sync_slot80(primary, &count, NULL);
        if (count != 0u) {
            if (!interface_forward_sync_items_reserve(count)) return;
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            for (i = 0u; i < count; ++i) {
                interface_array_forward_sync_slot(primary, 0xa0,
                    s_interface_forward_sync_items[i]);
            }
        }

        if (secondary != NULL) {
            uint32_t secondary_count = 0u;
            interface_array_forward_sync_slot80(secondary,
                                                 &secondary_count, NULL);
            if (secondary_count != 0u) {
                if (!interface_forward_sync_items_reserve(secondary_count))
                    return;
                interface_array_forward_sync_slot80(secondary,
                    &secondary_count, s_interface_forward_sync_items);
                for (i = 0u; i < secondary_count; ++i) {
                    void* item = s_interface_forward_sync_items[i];
                    interface_array_forward_sync_slot(primary, 0x48, item);
                    interface_array_forward_sync_slot(secondary_child, 0xa0,
                                                       item);
                }
            }
        }
        flags &= ~4u;
        if (s_interface_array_forward_sync_hooks.record_flags_set != NULL)
            s_interface_array_forward_sync_hooks.record_flags_set(
                s_interface_array_forward_sync_hooks.context, owner_link,
                flags);
        else if (sizeof(void*) == 4u)
            memcpy(record + 0x14, &flags, sizeof(flags));
    }
    else {
        count = 0u;
        interface_array_forward_sync_slot80(primary, &count,
                                             NULL);
        if (count != 0u) {
            if (!interface_forward_sync_items_reserve(count)) return;
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            for (i = 0u; i < count; ++i) {
                void* item = s_interface_forward_sync_items[i];
                interface_array_forward_sync_slot(secondary, 0x48, item);
                interface_array_forward_sync_slot(primary, 0xa0, item);
            }
        }
        selector = NULL;
        if (source != NULL &&
            s_interface_array_forward_sync_hooks.selector_get != NULL) {
            (void)s_interface_array_forward_sync_hooks.selector_get(
                s_interface_array_forward_sync_hooks.context,
                source_record, &selector);
        }
        else if (source != NULL && sizeof(void*) == 4u) {
            void* selector_owner = NULL;
            void* selector_data = NULL;
            memcpy(&selector_owner, source + 0x0c, sizeof(selector_owner));
            if (selector_owner != NULL) {
                memcpy(&selector, (unsigned char*)selector_owner + 0x08,
                       sizeof(selector));
                if (selector == NULL) {
                    memcpy(&selector_data,
                           (unsigned char*)selector_owner + 0x0c,
                           sizeof(selector_data));
                    if (selector_data != NULL)
                        memcpy(&selector,
                               (unsigned char*)selector_data + 0x0c,
                               sizeof(selector));
                }
            }
        }
        interface_array_forward_sync_slot(primary, 0x48, selector);
        flags |= 4u;
        if (s_interface_array_forward_sync_hooks.record_flags_set != NULL)
            s_interface_array_forward_sync_hooks.record_flags_set(
                s_interface_array_forward_sync_hooks.context, owner_link,
                flags);
        else if (sizeof(void*) == 4u)
            memcpy(record + 0x14, &flags, sizeof(flags));
    }
}
} // namespace lego

/* MeshRecordAllocateOrAppend (FUN_004748f0): use the in-memory mesh pool
 * when present; otherwise route through the legacy Direct3DRM record path. */
// FUNCTION: LEGORR 0x004748f0
int mesh_record_allocate_or_append(void* object, uint32_t vertex_count,
                                   uint32_t index_rows,
                                   uint32_t index_columns,
                                   const uint16_t* source_indices)
{
    MeshRecordPool* pool = NULL;
    if (object == NULL) return -1;
    if (s_mesh_record_append_ops.pool_get != NULL) {
        pool = s_mesh_record_append_ops.pool_get(
            s_mesh_record_append_ops.context, object);
    }
    if (pool != NULL) {
        return mesh_record_allocate_and_fill(pool, vertex_count, index_rows,
                                             index_columns, source_indices);
    }
    if (s_mesh_record_append_ops.legacy_append != NULL) {
        return s_mesh_record_append_ops.legacy_append(
            s_mesh_record_append_ops.context, object, vertex_count,
            index_rows, index_columns, source_indices);
    }
    return -1;
}

namespace lego {
// FUNCTION: LEGORR 0x00474a20
int animated_texture_frame_count(const void* texture)
{
    const unsigned char* record = static_cast<const unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void** vtable;
    void* frame_container;
    typedef void* (SCENE_RESOURCE_COM_CALL *FrameContainerGet)(void*);
    typedef int (SCENE_RESOURCE_COM_CALL *FrameCountGet)(void*);
    if (texture == 0) return 0;
    memcpy(&owner, record + 0x0c, sizeof(owner));
    if (owner == 0) return 0;
    memcpy(&direct_count, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_count));
    if (direct_count != 0) {
        int count;
        memcpy(&count, direct_count, sizeof(count));
        return count;
    }
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == 0) return 0;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0) return 0;
    if (vtable[0x1c / 4] != 0) {
        FrameContainerGet get_container =
            reinterpret_cast<FrameContainerGet>(vtable[0x1c / 4]);
        frame_container = get_container(interface_object);
        if (frame_container != 0) {
            int count;
            memcpy(&count, static_cast<unsigned char*>(frame_container) + 4,
                   sizeof(count));
            return count + 1;
        }
    }
    if (vtable[0x58 / 4] != 0) {
        FrameCountGet get_count = reinterpret_cast<FrameCountGet>(
            vtable[0x58 / 4]);
        return get_count(interface_object);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474a60
void animated_texture_frame_bind(void* texture, uint32_t frame_index, int mode)
{
    unsigned char* record = static_cast<unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void* resolved_interface;
    int resolved_index;
    void** vtable;
    unsigned int selector = 0x40;
    typedef void (SCENE_RESOURCE_COM_CALL *FrameBind)(
        void*, void*, unsigned int);
    if (texture == 0) return;
    memcpy(&owner, record + 0x0c, sizeof(owner));
    if (owner == 0) return;
    memcpy(&direct_count, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_count));
    if (direct_count != 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == 0) return;
    if (mode != 0) selector = frame_index;
    if (mode == 1) selector = 0x80;
    if (mode == 2) selector = 0x88;
    if (mode == 3) selector = 0x89;
    resolved_interface = interface_object;
    resolved_index = (int)frame_index;
    (void)animated_texture_frame_resolve(&resolved_interface,
                                         &resolved_index);
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x4c / 4] == 0) return;
    {
        FrameBind bind = reinterpret_cast<FrameBind>(vtable[0x4c / 4]);
        bind(interface_object, resolved_interface, selector);
    }
}
} // namespace lego

/* TerrainDetailEntryGetAtIndex (FUN_00474ae0): mesh paths delegate directly;
 * layer paths query child flags, with index zero reading the collection's
 * base enabled value. If no child array is available, the original returns
 * the requested index. */
// FUNCTION: LEGORR 0x00474ae0
uint32_t terrain_detail_entry_get_at_index(void* object,
                                           unsigned int index)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        return indexed_record_flag6c_bit0_get(mesh, (int)index);
    }
    else {
        void* layer = *(void**)((char*)owner + 8);
        void** layer_vtable = *(void***)layer;
        typedef void* (SURFACE_COM_CALL *LayerCollectionGetFn)(void*);
        void* children = ((LayerCollectionGetFn)
            layer_vtable[0x1c / sizeof(void*)])(layer);
        if (children != NULL) {
            if (index != 0u) {
                void* data = *(void**)children;
                void* child = *(void**)((char*)data - sizeof(uint32_t) +
                    (size_t)index * sizeof(void*));
                void** child_vtable = *(void***)child;
                typedef uint32_t (SURFACE_COM_CALL *LayerFlagsGetFn)(void*);
                uint32_t flags = ((LayerFlagsGetFn)
                    child_vtable[0x1c / sizeof(void*)])(child);
                return ~flags >> 31;
            }
            return *(uint32_t*)((char*)children + 0x0c);
        }
        return index;
    }
}
/* LwoLayerEnabledSet (FUN_00474b30): route mesh-backed entries to their
 * mesh setter; otherwise attach/detach the selected layer and maintain its
 * disabled high bit. Index zero uses the collection's base enabled field. */
// FUNCTION: LEGORR 0x00474b30
void lwo_layer_enabled_set(void* object, unsigned int index, int enabled,
                           const LwoLayerEnabledOps* ops)
{
    void* mesh;
    void* layer;
    int current_enabled;
    uint32_t flags;
    if (object == NULL || ops == NULL) return;
    mesh = ops->mesh_get != NULL
        ? ops->mesh_get(ops->context, object) : NULL;
    if (mesh != NULL) {
        if (ops->mesh_set != NULL)
            ops->mesh_set(ops->context, mesh, index, enabled);
        return;
    }
    if (ops->layer_get == NULL) return;
    layer = ops->layer_get(ops->context, object, index);
    if (layer == NULL) return;
    if (index == 0u) {
        current_enabled = ops->base_enabled_get != NULL
            ? ops->base_enabled_get(ops->context, layer) : 0;
        if (enabled != 0 && current_enabled == 0 &&
            ops->layer_attach != NULL) {
            ops->layer_attach(ops->context, object, layer);
        }
        else if (enabled == 0 && current_enabled != 0 &&
                 ops->layer_detach != NULL) {
            ops->layer_detach(ops->context, object, layer);
        }
        if (ops->base_enabled_set != NULL)
            ops->base_enabled_set(ops->context, layer, enabled);
        return;
    }
    if (ops->layer_flags_get == NULL || ops->layer_flags_set == NULL) return;
    flags = ops->layer_flags_get(ops->context, layer);
    current_enabled = (flags & 0x80000000u) == 0u;
    if (enabled != 0 && current_enabled == 0 && ops->layer_attach != NULL) {
        ops->layer_attach(ops->context, object, layer);
    }
    else if (enabled == 0 && current_enabled != 0 &&
             ops->layer_detach != NULL) {
        ops->layer_detach(ops->context, object, layer);
    }
    flags = (flags & 0x7fffffffu) |
            (enabled != 0 ? 0u : 0x80000000u);
    ops->layer_flags_set(ops->context, layer, flags);
}

namespace lego {
// FUNCTION: LEGORR 0x00474c00
int animated_texture_frame_resolve(void** interface_io,
                                   int* frame_index_io)
{
    void* interface_object;
    void** vtable;
    void* frame_container;
    void* frame_array;
    int frame_index;
    int frame_count;
    typedef void* (SCENE_RESOURCE_COM_CALL *FrameContainerGet)(void*);
    if (interface_io == 0 || frame_index_io == 0) return 0;
    interface_object = *interface_io;
    if (interface_object == 0) return 0;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x1c / 4] == 0) return 0;
    {
        FrameContainerGet get_container =
            reinterpret_cast<FrameContainerGet>(vtable[0x1c / 4]);
        frame_container = get_container(interface_object);
    }
    if (frame_container == 0) return 0;
    frame_index = *frame_index_io;
    if (frame_index != 0) {
        if (frame_index < 1) return 0;
        memcpy(&frame_count, static_cast<unsigned char*>(frame_container) + 4,
               sizeof(frame_count));
        if (frame_index > frame_count) return 0;
        memcpy(&frame_array, frame_container, sizeof(frame_array));
        if (frame_array == 0) return 0;
        memcpy(interface_io, static_cast<unsigned char*>(frame_array) +
               (size_t)(frame_index - 1) * sizeof(void*),
               sizeof(*interface_io));
    }
    *frame_index_io = 0;
    return 1;
}
} // namespace lego

// FUNCTION: LEGORR 0x00474c40
uint32_t lwo_layer_polygon_vertex_count(
    void* object, int block, uint32_t* field_out, uint32_t reserved,
    uint32_t* kind_out, uint32_t* count_out, uint32_t* indices_out)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        indexed_record_entry_read(mesh, block, field_out, reserved, kind_out,
                                  count_out, indices_out);
        return 0u;
    }
    else {
        void* interface_object = *(void**)((char*)owner + 8);
        int resolved_block = block;
        void** vtable;
        typedef int32_t (SURFACE_COM_CALL *PolygonVertexCountFn)(
            void*, int, uint32_t*, uint32_t, uint32_t*, uint32_t*,
            uint32_t*);
        (void)lego::animated_texture_frame_resolve(&interface_object,
                                                  &resolved_block);
        vtable = *(void***)interface_object;
        return (uint32_t)(((PolygonVertexCountFn)
            vtable[0x5c / sizeof(void*)])(
                interface_object, resolved_block, field_out, reserved,
                kind_out, count_out, indices_out) == 0);
    }
}

// FUNCTION: LEGORR 0x00474cc0
uint32_t lwo_layer_polygon_vertex_data_read(
    void* object, int block, int first_vertex, int count, void* output)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        indexed_geometry_data_copy_to_output(mesh, block, first_vertex,
                                             count, output);
        return 0u;
    }
    else {
        void* interface_object = *(void**)((char*)owner + 8);
        int resolved_block = block;
        void** vtable;
        typedef int32_t (SURFACE_COM_CALL *PolygonVertexDataReadFn)(
            void*, int, int, int, void*);
        (void)lego::animated_texture_frame_resolve(&interface_object,
                                                  &resolved_block);
        vtable = *(void***)interface_object;
        if (((PolygonVertexDataReadFn)vtable[0x60 / sizeof(void*)])(
                interface_object, resolved_block, first_vertex, count,
                output) != 0) {
            return 0xffffffffu;
        }
        return 0u;
    }
}

// FUNCTION: LEGORR 0x00474d30
uint32_t terrain_cell_quad_vertex_emit(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops)
{
    void* mesh;
    void* layer;
    unsigned int frame = 0u;
    if (object == NULL || ops == NULL || ops->mesh_get == NULL) return 0u;
    mesh = ops->mesh_get(ops->context, object);
    if (mesh != NULL) {
        return ops->mesh_terrain_quad_emit != NULL
            ? ops->mesh_terrain_quad_emit(ops->context, mesh, request) : 0u;
    }
    if (ops->layer_resolve == NULL ||
        ops->layer_terrain_quad_emit == NULL) return 0u;
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer == NULL) return 0u;
    return ops->layer_terrain_quad_emit(ops->context, layer, frame, request)
        == 0 ? 0xffffffffu : 0u;
}

namespace lego {
// FUNCTION: LEGORR 0x00474da0
void animated_texture_frame_set(void* texture, uint32_t frame_index,
                                const void* frame_state)
{
    unsigned char* texture_record = (unsigned char*)texture;
    void* owner;
    void* direct_pool;
    void* interface_object;
    void* frame_interface = NULL;
    void** vtable;
    int resolved_index = (int)frame_index;
    typedef int32_t (SCENE_RESOURCE_COM_CALL *QueryInterfaceFn)(
        void*, const void*, void**);
    typedef uint32_t (SCENE_RESOURCE_COM_CALL *ReleaseFn)(void*);
    typedef void (SCENE_RESOURCE_COM_CALL *FrameSurfaceSetFn)(
        void*, uint32_t, void*);
    static const unsigned char frame_surface_iid[16] = {
        0x09, 0xcb, 0x16, 0xeb, 0x71, 0xd2, 0xce, 0x11,
        0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1
    };

    if (texture_record == NULL) return;
    memcpy(&owner, texture_record + 0x0c, sizeof(owner));
    if (owner == NULL) return;
    memcpy(&direct_pool, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_pool));
    if (direct_pool != NULL) {
        animated_texture_frame_surface_set(direct_pool, (int)frame_index,
                                           frame_state);
        return;
    }
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == NULL) return;
    (void)animated_texture_frame_resolve(&interface_object,
                                         &resolved_index);
    if (frame_state != NULL) {
        void* source_interface = NULL;
        memcpy(&source_interface,
               static_cast<const unsigned char*>(frame_state) + 4,
               sizeof(source_interface));
        if (source_interface != NULL) {
            void** source_vtable = *static_cast<void***>(source_interface);
            if (source_vtable != NULL && source_vtable[0] != NULL) {
                QueryInterfaceFn query =
                    reinterpret_cast<QueryInterfaceFn>(source_vtable[0]);
                (void)query(source_interface, frame_surface_iid,
                            &frame_interface);
            }
        }
    }
    vtable = *static_cast<void***>(interface_object);
    if (vtable != NULL && vtable[0x54 / 4] != NULL) {
        FrameSurfaceSetFn set_surface =
            reinterpret_cast<FrameSurfaceSetFn>(vtable[0x54 / 4]);
        set_surface(interface_object, (uint32_t)resolved_index,
                    frame_interface);
    }
    if (frame_interface != NULL) {
        void** frame_vtable = *static_cast<void***>(frame_interface);
        if (frame_vtable != NULL && frame_vtable[2] != NULL) {
            ReleaseFn release = reinterpret_cast<ReleaseFn>(frame_vtable[2]);
            (void)release(frame_interface);
        }
    }
}
} // namespace lego

/* TerrainSurfacePropertySet (FUN_00474e30): mesh-backed objects already
 * receive material state through their mesh; regular layers get property
 * bit 4 set or cleared on the resolved animated layer frame. */
// FUNCTION: LEGORR 0x00474e30
void terrain_surface_property_set(void* object, unsigned int layer_index,
                                  int enabled,
                                  const TerrainSurfacePropertyOps* ops)
{
    unsigned int frame = 0u;
    void* layer;
    if (object == NULL || ops == NULL || ops->layer_resolve == NULL ||
        ops->property_set == NULL) return;
    if (ops->has_mesh != NULL && ops->has_mesh(ops->context, object) != 0) {
        return;
    }
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer != NULL) {
        ops->property_set(ops->context, layer, frame,
                          enabled != 0 ? 4u : 0u);
    }
}

namespace unported_ledger_stubs {
// RenderObjectHierarchyFloat3Set
// FUNCTION: LEGORR 0x00474e90
uint32_t ledger_stub_00474e90_RenderObjectHierarchyFloat3Set(
    void* scene_object, float x, float y, float z)
{
    float xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    return render_object_hierarchy_float3_set(scene_object, xyz);
}
} // namespace unported_ledger_stubs

/* SceneObjectBoundsUnion (FUN_00474f10): the root's bounds initialize the
 * output, then each child contributes a componentwise min/max. The original
 * obtains both the root and child collection through scene-interface calls;
 * those accessors stay replaceable while the union behavior is local. */
// FUNCTION: LEGORR 0x00474f10
uint32_t scene_object_bounds_union(void* scene_object,
                                  float bounds_min_max[6])
{
    SceneObjectBoundsHooks* hooks = &s_scene_object_bounds_hooks;
    void* root;
    const void* children;
    uint32_t count = 0;
    uint32_t i;
    float child_bounds[6];

    if (scene_object == NULL || bounds_min_max == NULL ||
        hooks->root_get == NULL || hooks->bounds_query == NULL) {
        return 0;
    }
    root = hooks->root_get(scene_object, hooks->context);
    if (root == NULL) return 0;
    hooks->bounds_query(root, bounds_min_max, hooks->context);
    if (hooks->children_get == NULL || hooks->child_get == NULL) return 0;
    children = hooks->children_get(root, &count, hooks->context);
    if (children == NULL) return 0;

    for (i = 0; i < count; i++) {
        void* child = hooks->child_get(children, i, hooks->context);
        int axis;
        if (child == NULL) continue;
        hooks->bounds_query(child, child_bounds, hooks->context);
        for (axis = 0; axis < 3; axis++) {
            if (child_bounds[axis] < bounds_min_max[axis]) {
                bounds_min_max[axis] = child_bounds[axis];
            }
            if (child_bounds[axis + 3] > bounds_min_max[axis + 3]) {
                bounds_min_max[axis + 3] = child_bounds[axis + 3];
            }
        }
    }
    return 0;
}

/* SceneMeshColorSet (FUN_00474ff0) — see view.hpp. Same inner-record ride
 * as the alpha sibling; mode three lands the triple in the block's +0x34
 * slot through the ported color-vector setter. */
// FUNCTION: LEGORR 0x00474ff0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_mesh_color_set(void* object, int block, float red,
                                             float green, float blue)
{
    __asm {
        /* These checks retain the port's test-pinned null-chain behavior. */
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je scene_mesh_color_null_return
        mov ecx, dword ptr [eax + 0x0c]
        test ecx, ecx
        je scene_mesh_color_null_return
        mov eax, dword ptr [ecx + 0x0c]
        test eax, eax
        je scene_mesh_color_null_return

        /* Corpus call/stack schedule from FUN_00474ff0. */
        mov edx, dword ptr [esp + 0x14]
        mov ecx, dword ptr [esp + 0x10]
        push 3
        push edx
        mov edx, dword ptr [esp + 0x14]
        push ecx
        mov ecx, dword ptr [esp + 0x14]
        push edx
        push ecx
        push eax
        call render_object_color_vector_set
        add esp, 0x18
    scene_mesh_color_null_return:
        ret
    }
}
#else
void scene_mesh_color_set(void* object, int block, float red, float green,
                          float blue)
{
    void* mesh = NULL;
    void* record = NULL;
    if (object != NULL) {
        mesh = *(void* const*)((const char*)object + 0x0c);
    }
    if (mesh != NULL) {
        record = *(void* const*)((const char*)mesh + 0x0c);
    }
    if (record == NULL) {
        return; /* pinned deviation: the corpus strides both links blindly */
    }
    render_object_color_vector_set(record, block, red, green, blue, 3);
}
#endif
/* SceneMeshColorAndAlphaSet (FUN_00475020) — see view.hpp. */
// FUNCTION: LEGORR 0x00475020
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_mesh_color_and_alpha_set(void* object, int block,
                                                       float red, float green,
                                                       float blue, float alpha)
{
    __asm {
        /* These two checks preserve the port's test-pinned null behavior;
         * the corpus blindly follows both links. */
        mov eax, dword ptr [esp + 4]
        push esi
        test eax, eax
        je scene_null_return
        mov ecx, dword ptr [eax + 0x0c]
        test ecx, ecx
        je scene_null_return
        mov esi, dword ptr [ecx + 0x0c]
        test esi, esi
        je scene_null_return
        mov edx, dword ptr [esp + 0x18]
        mov eax, dword ptr [esp + 0x14]
        mov ecx, dword ptr [esp + 0x10]
        push edi
        mov edi, dword ptr [esp + 0x10]
        push 0
        push edx
        push eax
        push ecx
        push edi
        push esi
        call render_object_color_vector_set
        mov edx, dword ptr [esp + 0x38]
        add esp, 0x18
        push 4
        push edx
        push edi
        push esi
        call screen_effect_item_opacity_set
        add esp, 0x10
        pop edi
        pop esi
        ret
    scene_null_return:
        pop esi
        ret
    }
}
#else
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue, float alpha)
{
    if (object == NULL) {
        return; /* pinned deviation: the corpus strides the object blindly */
    }
    void* mesh = *(void**)((char*)object + 0xc);
    if (mesh == NULL) {
        return; /* pinned deviation: the corpus strides the mesh link blindly */
    }
    void* record = *(void**)((char*)mesh + 0xc);
    if (record == NULL) {
        return; /* corpus: the single gate, the record test at 0x47502b */
    }
    render_object_color_vector_set(record, block, red, green, blue, 0);
    screen_effect_item_opacity_set(record, block, alpha, 4);
}
#endif
/* SceneObjectCoordinateTransformInPlace (FUN_00475070), interface slot
 * +0x108 as recovered in Finding 337. */
// FUNCTION: LEGORR 0x00475070
void scene_object_coordinate_transform_in_place(void* object,
                                                void* coordinates,
                                                void* transform)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *CoordinateTransformFn)(void*, void*, void*);
    ((CoordinateTransformFn)vtable[0x108 / 4])(scene_object, coordinates,
                                                transform);
#else
    SceneObjectCoordinateInterface* const scene =
        (SceneObjectCoordinateInterface*)*(void**)object;
    scene->coordinate_transform_in_place(coordinates, transform);
#endif
}
/* SceneObjectCoordinateTransformToOutput (FUN_00475090), interface slot
 * +0x84 as recovered in Finding 337. */
// FUNCTION: LEGORR 0x00475090
void scene_object_coordinate_transform_to_output(void* object,
                                                  void* input, void* output)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *CoordinateTransformFn)(void*, void*, void*);
    ((CoordinateTransformFn)vtable[0x84 / 4])(scene_object, input, output);
#else
    SceneObjectCoordinateInterface* const scene =
        (SceneObjectCoordinateInterface*)*(void**)object;
    scene->coordinate_transform_to_output(input, output);
#endif
}
/* RenderObjectSetPackedRgbaFromFloatChannels (FUN_004750b0): convert the
 * four caller-supplied float channels in reference order, then update the
 * render interface stored in the kind-five mesh record. */
// FUNCTION: LEGORR 0x004750b0
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue, float alpha)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    const uint32_t red_byte = (uint32_t)crt_float_to_long(red * 255.0f);
    const uint32_t green_byte = (uint32_t)crt_float_to_long(green * 255.0f);
    const uint32_t blue_byte = (uint32_t)crt_float_to_long(blue * 255.0f);
    const uint32_t alpha_byte = (uint32_t)crt_float_to_long(alpha * 255.0f);
    const uint32_t packed = (red_byte << 24) | (green_byte << 16) |
                            (blue_byte << 8) | alpha_byte;
    if (*(uint32_t*)((char*)scene_object + 0x10) == 5u) {
        void* mesh = *(void**)((char*)scene_object + 0x0c);
        void* render_object = *(void**)((char*)mesh + 4);
        void** vtable = *(void***)render_object;
        typedef void (VIEW_COM_CALL *PackedColorSetFn)(void*, uint32_t);
        ((PackedColorSetFn)vtable[0x30 / 4])(render_object, packed);
    }
#else
    RenderPackedRgbaHooks* hooks = &s_render_packed_rgba_hooks;
    void* mesh;
    void* render_object;
    const uint32_t red_byte =
        (uint32_t)crt_float_to_long(red * 255.0f);
    const uint32_t green_byte =
        (uint32_t)crt_float_to_long(green * 255.0f);
    const uint32_t blue_byte =
        (uint32_t)crt_float_to_long(blue * 255.0f);
    const uint32_t alpha_byte =
        (uint32_t)crt_float_to_long(alpha * 255.0f);
    const uint32_t packed = (red_byte << 24) | (green_byte << 16) |
                            (blue_byte << 8) | alpha_byte;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 5u ||
        hooks->packed_set == NULL) return;
    mesh = *(void**)((char*)scene_object + 0x0c);
    if (mesh == NULL) return;
    render_object = *(void**)((char*)mesh + 4);
    if (render_object == NULL) return;
    hooks->packed_set(render_object, packed, hooks->context);
#endif
}

/* DDiSceneAdvanceByTime target: the retail body assumes a valid scene object
 * and returns the x87 result left by its frame-update call. */
// FUNCTION: LEGORR 0x00475120
float ddiscene_advance_by_time_target(void* scene_object, float delta)
{
    const float current = ddiscene_object_length_get(scene_object);
    const float next = current + delta;
    return ddiscene_frame_update_and_callbacks(scene_object, next);
}
// FUNCTION: LEGORR 0x00475140
/* DDiSceneFrameUpdateAndCallbacks (FUN_00475140): select the kind-specific
 * frame, consume the action-entry sample bit, update its timing record, run
 * timed dispatch, and signal the registered end callback on frame overflow. */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time)
{
    void* frame_interface = NULL;
    void* action_slot;
    const char* action_name;
    uint32_t kind;
    uint32_t flags;
    uint32_t timing_suppressed;
    const char* sample_name;
    uint32_t frame_count;
    uint32_t last_frame;
    float length;
    float remainder = 0.0f;
    float last_frame_time;
    void* timing_record;

    if (scene_object == NULL) return 0.0f;
    kind = *(uint32_t*)((char*)scene_object + 0x10);
    flags = *(uint32_t*)((char*)scene_object + 0x14);
    if (kind == 4u) {
        action_slot = *(void**)((char*)scene_object + 0x0c);
        action_name = action_slot != NULL ? *(const char**)action_slot : NULL;
        if (action_name != NULL) {
            frame_interface = ddiscene_frame_record_find(
                *(void**)((char*)scene_object + 4), action_name, 0);
        }
        if (frame_interface != NULL) {
            timing_record = (void*)(uintptr_t)
                ddiscene_action_record_field_1c_get(frame_interface);
        }
        if (frame_interface != NULL && (flags & 2u) != 0u) {
            sample_name = ddiscene_animation_sample_name_get(frame_interface);
            if (sample_name != NULL && s_ddiscene_sample_enabled != 0) {
                ddiscene_sample_hook_dispatch(sample_name, scene_object);
            }
            *(uint32_t*)((char*)scene_object + 0x14) = flags & ~2u;
            flags &= ~2u;
        }
    }
    else if (kind == 3u) {
        frame_interface = *(void**)((char*)scene_object + 4);
        if (frame_interface != NULL) {
            timing_record = (void*)(uintptr_t)
                ddiscene_action_record_field_1c_get(frame_interface);
        }
    }
    if (frame_interface == NULL) return 0.0f;

    frame_count = ddiscene_action_record_field_0c_get(frame_interface);
    length = ddiscene_object_length_get(scene_object);

    /* FUN_00475f70 updates the frame interface's stored time value. */
    ddiscene_animation_hook_create(frame_interface, 0, NULL, NULL, 0,
                                   NULL, (const uint32_t*)&time, 0, 0, NULL);

    timing_suppressed = flags & 8u;
    if (frame_count != 0u) {
        --frame_count;
        if ((float)frame_count < time) {
            remainder = time - (float)frame_count;
            timing_suppressed = 0u;
        }
    }
    if (timing_suppressed == 0u) {
        last_frame = ddiscene_animation_last_frame_get(frame_interface);
        last_frame_time = (float)last_frame;
        if (frame_count != 1u) {
            ddiscene_timed_hook_dispatch(timing_record, time, NULL);
            if (s_ddiscene_end_hook != NULL && last_frame != 0u &&
                length < last_frame_time && last_frame_time <= time) {
                s_ddiscene_end_hook(scene_object, s_ddiscene_end_context);
                return remainder;
            }
        }
    }
    else {
        *(uint32_t*)((char*)scene_object + 0x14) = flags | 0x20u;
    }
    return remainder;
}
// FUNCTION: LEGORR 0x00475300
/* DDiSceneApplyActFrameTiming (FUN_00475300): only records with the pending
 * timing bit dispatch their current action frame's timed hook. Kind 4 finds
 * the selected frame through channel 0; kind 3 stores that channel directly. */
void ddiscene_apply_act_frame_timing(void* scene_object)
{
    void* selected = scene_object;
    uint32_t hook_record = (uint32_t)(uintptr_t)scene_object;
    float time;
    memcpy(&time, &scene_object, sizeof(time));
    if ((*(uint32_t*)((char*)scene_object + 0x14) & 0x20u) != 0u) {
        uint32_t kind = *(uint32_t*)((char*)scene_object + 0x10);
        if (kind == 4u) {
            selected = ddiscene_frame_record_find(
                scene_object,
                **(const char***)((char*)scene_object + 0x0c), 0);
            if (selected != NULL) {
                hook_record = ddiscene_action_record_field_1c_get(selected);
                time = ddiscene_animation_duration_get(selected);
            }
        }
        else if (kind == 3u) {
            selected = *(void**)((char*)scene_object + 4);
            hook_record = ddiscene_action_record_field_1c_get(selected);
            time = ddiscene_animation_duration_get(selected);
        }
        if (hook_record != 0u) {
            ddiscene_timed_hook_dispatch((void*)hook_record, time, NULL);
        }
    }
}

/* DDiSceneObjectLengthGet (FUN_00475390): kind 4 resolves its selected
 * ActFrame record through channel 0; kind 3 already carries the channel. */
// FUNCTION: LEGORR 0x00475390
float ddiscene_object_length_get(void* scene_object)
{
    const uint32_t kind = *(uint32_t*)((char*)scene_object + 0x10);
    if (kind == 4u) {
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        void* interface_object = ddiscene_frame_record_find(
            *(void**)((char*)scene_object + 4), *(const char**)action_slot, 0);
        if (interface_object != NULL)
            return ddiscene_animation_duration_get(interface_object);
    }
    else if (kind == 3u) {
        return ddiscene_animation_duration_get(
            *(void**)((char*)scene_object + 4));
    }
    return 0.0f;
}

/* DDiSceneObjectFramePositionGet (FUN_004753f0): follow the same kind
 * dispatch, then read the selected frame record's +0x0c field. */
// FUNCTION: LEGORR 0x004753f0
uint32_t ddiscene_object_frame_position_get(void* scene_object)
{
    void* interface_object = NULL;
    const uint32_t kind = *(uint32_t*)((char*)scene_object + 0x10);
    if (kind == 4u) {
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        interface_object = ddiscene_frame_record_find(
            *(void**)((char*)scene_object + 4), *(const char**)action_slot, 0);
    }
    else if (kind == 3u) {
        interface_object = *(void**)((char*)scene_object + 4);
    }
    return interface_object != NULL
        ? ddiscene_action_record_field_0c_get(interface_object) : 0u;
}

// FUNCTION: LEGORR 0x00475430
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* normalized_object = NULL;
    const void* normalized_frame = NULL;
    void** vtable;
    typedef long (VIEW_COM_CALL *PositionSetFn)(void*, void*, float, float,
                                                float);
    scene_object_arguments_normalize((const void**)object,
        (const void**)alternate_frame,
        &normalized_object, &normalized_frame);
    vtable = *(void***)normalized_object;
    return ((PositionSetFn)vtable[0xf0 / 4])(normalized_object,
        (void*)normalized_frame, x, y, z) == 0;
#else
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene =
        ddiscene_pose_interface_resolve(object, alternate_frame,
                                        &normalized_frame);
    if (scene == NULL) return 0;
    scene->position_set(normalized_frame, x, y, z);
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00475470
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float parameter_3,
                                         float parameter_4,
                                         float parameter_5,
                                         float parameter_6,
                                         float parameter_7,
                                         float parameter_8)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* normalized_object = NULL;
    const void* normalized_frame = NULL;
    void** vtable;
    typedef long (VIEW_COM_CALL *PoseMethodDispatchFn)(
        void*, void*, float, float, float, float, float, float);
    scene_object_arguments_normalize((const void**)object,
        (const void**)alternate_frame,
        &normalized_object, &normalized_frame);
    vtable = *(void***)normalized_object;
    return ((PoseMethodDispatchFn)vtable[0xec / 4])(normalized_object,
        (void*)normalized_frame, parameter_3, parameter_4, parameter_5,
        parameter_6, parameter_7, parameter_8) == 0;
#else
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene =
        ddiscene_pose_interface_resolve(object, alternate_frame,
                                        &normalized_frame);
    if (scene == NULL) return 0;
    scene->pose_method_dispatch(normalized_frame, parameter_3, parameter_4,
        parameter_5, parameter_6, parameter_7, parameter_8);
    return 1;
#endif
}
// FUNCTION: LEGORR 0x004754c0
#if !defined(LEGORRR_BACKEND_MINIWIN)
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3])
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* normalized_object = NULL;
    const void* normalized_frame = NULL;
    void** vtable;
    typedef long (VIEW_COM_CALL *PositionGetFn)(void*, void*, float*);
    scene_object_arguments_normalize((const void**)object,
        (const void**)alternate_frame,
        &normalized_object, &normalized_frame);
    vtable = *(void***)normalized_object;
    return ((PositionGetFn)vtable[0x60 / 4])(normalized_object,
        (void*)normalized_frame, position) == 0;
#else
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene;
    if (position == NULL) return 0;
    scene = ddiscene_pose_interface_resolve(object, alternate_frame,
                                            &normalized_frame);
    if (scene == NULL) return 0;
    scene->position_get(normalized_frame, position);
    return 1;
#endif
}
#else
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3])
{
    /* Native Containers and the null frames their part search returns
     * (container_native.hpp). The Creature unit reaches its other
     * Container calls through container_route.h; this one is also called
     * from C++ (the work-object position), so it is a whole alternative
     * definition. It follows the matching build's branch, which Reccmp
     * maps by the line below the annotation (finding 555). */
    if (alternate_frame == NULL && container_native_object_known(object))
        return container_native_position_get(object, position);
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* normalized_object = NULL;
    const void* normalized_frame = NULL;
    void** vtable;
    typedef long (VIEW_COM_CALL *PositionGetFn)(void*, void*, float*);
    scene_object_arguments_normalize((const void**)object,
        (const void**)alternate_frame,
        &normalized_object, &normalized_frame);
    vtable = *(void***)normalized_object;
    return ((PositionGetFn)vtable[0x60 / 4])(normalized_object,
        (void*)normalized_frame, position) == 0;
#else
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene;
    if (position == NULL) return 0;
    scene = ddiscene_pose_interface_resolve(object, alternate_frame,
                                            &normalized_frame);
    if (scene == NULL) return 0;
    scene->position_get(normalized_frame, position);
    return 1;
#endif
}
#endif

// FUNCTION: LEGORR 0x00475500
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3])
{
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene;
    void** vtable;
    typedef long (VIEW_COM_CALL *DdiSceneOrientationGetFn)(
        void* object, void* frame, float first[3], float second[3]);
    if (first == NULL || second == NULL) return 0;
    scene = ddiscene_pose_interface_resolve(object, alternate_frame,
                                            &normalized_frame);
    if (scene == NULL) return 0;
    vtable = *(void***)scene;
    return ((DdiSceneOrientationGetFn)vtable[0x7c / 4u])(
        scene, normalized_frame, first, second) == 0;
}
/* RenderObjectRotateAxis (FUN_00475580), vtable slot +0x44. */
// FUNCTION: LEGORR 0x00475580
void render_object_rotate_axis(void* object, uint32_t mode, float x, float y,
                               float z, float angle)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *RotateAxisFn)(void*, uint32_t, float, float,
                                                float, float);
    ((RotateAxisFn)vtable[0x44 / 4])(scene_object, mode, x, y, z, angle);
#else
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->rotate_axis(mode, x, y, z, angle);
#endif
}

/* SceneFrameScaleSet (FUN_004755b0), vtable slot +0x40. */
// FUNCTION: LEGORR 0x004755b0
void scene_frame_scale_set(void* object, uint32_t mode, float x, float y,
                           float z)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *FrameScaleSetFn)(void*, uint32_t, float,
                                                   float, float);
    ((FrameScaleSetFn)vtable[0x40 / 4])(scene_object, mode, x, y, z);
#else
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->frame_scale_set(mode, x, y, z);
#endif
}

/* CameraTeleportOrOffset (FUN_004755e0), vtable slot +0x3c. */
// FUNCTION: LEGORR 0x004755e0
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y, float z)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *CameraTeleportOrOffsetFn)(void*, uint32_t,
                                                            float, float,
                                                            float);
    ((CameraTeleportOrOffsetFn)vtable[0x3c / 4])(
        scene_object, mode, x, y, z);
#else
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->slot_3c(mode, x, y, z);
#endif
}
/* RenderObjectSetIdentityTransform (FUN_00475610). */
// FUNCTION: LEGORR 0x00475610
void render_object_set_identity_transform(void* object)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    volatile float matrix[16];
    matrix[0] = 1.0f;
    matrix[4] = 0.0f;
    matrix[8] = 0.0f;
    matrix[12] = 0.0f;
    matrix[1] = 0.0f;
    matrix[5] = 1.0f;
    matrix[9] = 0.0f;
    matrix[13] = 0.0f;
    matrix[2] = 0.0f;
    matrix[6] = 0.0f;
    matrix[10] = 1.0f;
    matrix[14] = 0.0f;
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = 0.0f;
    matrix[15] = 1.0f;
    scene_object_transform_matrix_set(object, 0u, (const float*)matrix);
#else
    float matrix[16];
    matrix4_set_identity(matrix);
    scene_object_transform_matrix_set(object, 0u, matrix);
#endif
}
/* SceneObjectTransformMatrixSet (FUN_004756b0). The wrapper stores the
 * renderer interface pointer in its first word; the corpus dispatches slot
 * 0x38 with the caller's mode and matrix. */
// FUNCTION: LEGORR 0x004756b0
void scene_object_transform_matrix_set(void* object, uint32_t mode,
                                       const float* matrix)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* scene_object = *(void**)object;
    void** vtable = *(void***)scene_object;
    typedef void (VIEW_COM_CALL *MatrixSetFn)(void*, uint32_t,
                                               const float*);
    ((MatrixSetFn)vtable[0x38 / 4])(scene_object, mode, matrix);
#else
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->transform_matrix_set(mode, matrix);
#endif
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x004756d0
extern "C" float ledger_stub_004756d0_VideoObjectAspectRatioGet(void* video_object)
{
    return video_object_aspect_ratio_get(video_object);
}
} // namespace unported_ledger_stubs

/* DDiSceneObjectAttachOrUnlink (FUN_00475710): pairs both stored interfaces
 * against a source pair, or asks the backend to unlink each current one. */
// FUNCTION: LEGORR 0x00475710
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record)
{
    void* first;
    void* second;
    if (pair_record == NULL) return;
    memcpy(&first, pair_record, sizeof(first));
    memcpy(&second, (char*)pair_record + 2 * sizeof(void*), sizeof(second));
    if (source_pair_record == NULL) {
        if (first != NULL &&
            s_render_interface_pair_hooks.interface_unlink != NULL) {
            s_render_interface_pair_hooks.interface_unlink(
                first, s_render_interface_pair_hooks.context);
        }
        if (second != NULL &&
            s_render_interface_pair_hooks.interface_unlink != NULL) {
            s_render_interface_pair_hooks.interface_unlink(
                second, s_render_interface_pair_hooks.context);
        }
        return;
    }
    {
        void* source_first;
        void* source_second;
        memcpy(&source_first, source_pair_record, sizeof(source_first));
        memcpy(&source_second,
               (char*)source_pair_record + 2 * sizeof(void*),
               sizeof(source_second));
        render_interface_pair_update(source_first, first);
        render_interface_pair_update(source_second, second);
    }
}
// FUNCTION: LEGORR 0x004757a0
void* ddiscene_context_get(void* scene_object)
{
    void* owner_interface = NULL;
    void* result;
    SceneContextHooks* const hooks = &s_scene_context_hooks;
    if (scene_object == NULL || scene_object == hooks->shared_scene ||
        hooks->owner_link_ensure == NULL) {
        return NULL;
    }
    if (hooks->owner_interface_get != NULL) {
        if (!hooks->owner_interface_get(scene_object, &owner_interface,
                                        hooks->context)) return NULL;
    }
    else {
        typedef void (VIEW_COM_CALL *OwnerInterfaceGet)(void*, void**);
        void* scene_interface = NULL;
        void** vtable;
        OwnerInterfaceGet owner_get;
        memcpy(&scene_interface, scene_object, sizeof(scene_interface));
        if (scene_interface == NULL) return NULL;
        vtable = *static_cast<void***>(scene_interface);
        if (vtable == NULL || vtable[0x5c / 4] == NULL) return NULL;
        owner_get = reinterpret_cast<OwnerInterfaceGet>(vtable[0x5c / 4]);
        owner_get(scene_interface, &owner_interface);
    }
    if (owner_interface == NULL) return NULL;
    result = scene_object_owner_link_ensure(owner_interface);
    if (hooks->interface_release != NULL) {
        hooks->interface_release(owner_interface, hooks->context);
    }
    else {
        void** vtable = *static_cast<void***>(owner_interface);
        typedef uint32_t (VIEW_COM_CALL *InterfaceRelease)(void*);
        if (vtable != NULL && vtable[2] != NULL) {
            InterfaceRelease release =
                reinterpret_cast<InterfaceRelease>(vtable[2]);
            (void)release(owner_interface);
        }
    }
    return result;
}

/* TypedChildResourceScalarGet (FUN_004757f0): only type-4 scene objects with
 * a non-null resource name can resolve a child property. */
// FUNCTION: LEGORR 0x004757f0
float typed_child_resource_scalar_get(void* scene_object)
{
    if (*(uint32_t*)((char*)scene_object + 0x10) == 4u) {
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        if (action_slot != NULL) {
            const char* action_name = *(const char**)action_slot;
            if (action_name != NULL) {
                void* record = ddiscene_frame_record_find(
                    scene_object, action_name, 0);
                if (record != NULL) {
                    return scene_property_float_at14_get(record);
                }
            }
        }
    }
    return 0.0f;
}

/* SceneObjectFirstNonKind6ChildGet (FUN_00475830): walk the child-interface
 * chain until its owner property has a kind other than six. */
// FUNCTION: LEGORR 0x00475830
void* scene_object_first_non_kind6_child_get(void* child_interface)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    void* current = child_interface;
    void* result = NULL;
    while (current != NULL && result == NULL) {
        const uint32_t property_value =
            scene_property_primary_value_get(current);
        void* next = NULL;
        void** vtable;
        typedef void (VIEW_COM_CALL *ChildGetFn)(void*, void**);
        if (property_value != 0u &&
            *(uint32_t*)((char*)(size_t)property_value + 0x10) != 6u) {
            result = (void*)(size_t)property_value;
        }
        vtable = *(void***)current;
        ((ChildGetFn)vtable[0x5c / 4])(current, &next);
        current = next;
        if (current != NULL) {
            void** current_vtable = *(void***)current;
            typedef uint32_t (VIEW_COM_CALL *SceneInterfaceReleaseFn)(void*);
            ((SceneInterfaceReleaseFn)current_vtable[2])(current);
        }
    }
    return result;
#else
    SceneChildSearchHooks* hooks = &s_scene_child_search_hooks;
    void* current = child_interface;
    void* result = NULL;
    uint32_t guard = 0;
    while (current != NULL && result == NULL && guard < 4096u) {
        void* property;
        void* next;
        if (hooks->property_get == NULL || hooks->next_get == NULL) return NULL;
        property = hooks->property_get(current, hooks->context);
        if (property != NULL && *(uint32_t*)((char*)property + 0x10) != 6u) {
            result = property;
        }
        next = hooks->next_get(current, hooks->context);
        if (next != NULL && hooks->release != NULL) {
            hooks->release(next, hooks->context);
        }
        current = next;
        ++guard;
    }
    return result;
#endif
}

/* DDiSceneObjectArgumentsNormalize (FUN_00475900) — resolve the dispatch
 * pair: the object slot rides through, and the second slot falls back to
 * the root object's first slot when the caller passes NULL. */
// FUNCTION: LEGORR 0x00475900
void scene_object_arguments_normalize(const void** obj, const void** alt,
                                      void** out_obj, const void** out_alt)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__i386__)
    *out_obj = (void*)*obj;
    if (alt == NULL) {
        *out_alt = *(const void**)s_scene_root;
    }
    else {
        *out_alt = *alt;
    }
#else
    if (obj != NULL && out_obj != NULL) {
        *out_obj = (void*)*obj; /* corpus: blind dereference */
    }
    if (out_alt == NULL) {
        return;
    }
    if (alt != NULL) {
        *out_alt = *alt; /* corpus: blind dereference */
        return;
    }
    /* Deviation: a missing root answers NULL; the corpus strides it. */
    const void* root_slot = NULL;
    if (s_scene_root != NULL) {
        __builtin_memcpy((void*)&root_slot, s_scene_root, sizeof root_slot);
    }
    *out_alt = root_slot;
#endif
}

// FUNCTION: LEGORR 0x00475930
int entity_kind_lookup(const char* kind, int* notexture)
{
    if (notexture != NULL) {
        *notexture = 0;
    }
    if (kind == NULL || *kind == '\0') {
        return ENTITY_KIND_UNKNOWN;
    }

    /* Tokenize on the corpus ':' delimiter. The kind comes from the first
     * token and NOTEXTURE, when present, is checked against the second. */
    char buf[64];
    snprintf(buf, sizeof buf, "%s", kind);
    char* colon = strchr(buf, ':');
    if (colon != NULL) {
        *colon = '\0';
        char* option = colon + 1;
        char* next = strchr(option, ':');
        if (next != NULL) {
            *next = '\0';
        }
        if (strcasecmp_lite(option, "NOTEXTURE") == 0 && notexture != NULL) {
            *notexture = 1;
        }
    }

    for (int i = 0; i < 9; i++) {
        if (s_kinds[i] != NULL && strcasecmp_lite(s_kinds[i], buf) == 0) {
            return i;
        }
    }
    return ENTITY_KIND_UNKNOWN;
}

namespace lego {
/* The reference's DDiSceneObjectArrayGrow allocates 1<<level records at
 * stride 0x2c and links them through the word at +0x28. The reserved words
 * keep that x86 record layout while pointers widen in the portable build. */
// FUNCTION: LEGORR 0x004759f0
void ddiscene_object_array_grow(void)
{
    uint32_t count;
    uint32_t i;
    DDiSceneObjectRecord* block;
    DDiSceneObjectRecord* record;
    if (s_scene_object_level_count >= DDI_SCENE_POOL_BANKS) return;
    count = 1u << s_scene_object_level_count;
    if ((size_t)count > (size_t)0x7fffffff / sizeof(DDiSceneObjectRecord))
        return;
    block = (DDiSceneObjectRecord*)engine_heap_alloc(
        (long)((size_t)count * sizeof(DDiSceneObjectRecord)));
    if (block == 0) return;
    s_scene_object_blocks[s_scene_object_level_count] = block;
    ++s_scene_object_level_count;
    for (i = 0; i < count; ++i) {
        record = block + i;
        memset(record, 0, sizeof(*record));
        record->next = (i + 1 < count) ? (record + 1) : s_scene_object_free_head;
    }
    s_scene_object_free_head = block;
}
} // namespace lego

/* SceneActionRecordActFrameScan (FUN_00475a70): enumerate both type-four
 * animation channels and fill the caller's optional record, frame-index, and
 * owned-name arrays in the same order. */
// FUNCTION: LEGORR 0x00475a70
unsigned int scene_action_record_act_frame_scan(void* scene_object,
                                                void** records,
                                                int* frame_indices,
                                                char** names)
{
#if defined(_MSC_VER)
#define SCENE_SCAN_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_SCAN_STDCALL __attribute__((stdcall))
#else
#define SCENE_SCAN_STDCALL
#endif
    typedef unsigned int (SCENE_SCAN_STDCALL *ActionCountMethod)(void*);
    typedef void (SCENE_SCAN_STDCALL *ActionRecordMethod)(
        void*, unsigned int, void**);
    unsigned int slot, found = 0;
    SceneActFrameScanHooks* hooks = &s_scene_act_frame_scan_hooks;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 4u ||
        hooks->action_name_get == NULL)
        return 0;
    for (slot = 0; slot < 2; ++slot) {
        void* animation;
        if (hooks->animation_interface_get != NULL) {
            animation = hooks->animation_interface_get(
                scene_object, slot, hooks->context);
        }
        else {
            memcpy(&animation,
                   (char*)scene_object + (slot == 0u ? 4u : 8u),
                   sizeof(animation));
        }
        unsigned int count, index;
        if (animation == NULL) continue;
        if (hooks->action_count_get != NULL) {
            count = hooks->action_count_get(animation, hooks->context);
        }
        else {
            void** vtable = *(void***)animation;
            if (vtable == NULL || vtable[3] == NULL) continue;
            count = ((ActionCountMethod)vtable[3])(animation);
        }
        for (index = 0; index < count; ++index) {
            const char* name = hooks->action_name_get(animation, index,
                                                       hooks->context);
            if (scene_act_frame_name_match(name)) {
                void* action_record = NULL;
                if (records != NULL || frame_indices != NULL) {
                    if (hooks->action_record_get != NULL) {
                        action_record = hooks->action_record_get(
                            animation, index, hooks->context);
                    }
                    else {
                        void** vtable = *(void***)animation;
                        if (vtable != NULL && vtable[4] != NULL) {
                            ((ActionRecordMethod)vtable[4])(
                                animation, index, &action_record);
                        }
                    }
                }
                if (records != NULL) {
                    records[found] = action_record;
                }
                if (frame_indices != NULL) {
                    int value = 0;
                    if (action_record != NULL) {
                        value = (int)ddiscene_action_record_field_1c_get(
                            action_record);
                    }
                    else if (hooks->action_value_get != NULL) {
                        hooks->action_value_get(animation, index, &value,
                                                hooks->context);
                    }
                    frame_indices[found] = value;
                }
                if (names != NULL) {
                    size_t length = strlen(name);
                    char* copy = (char*)engine_heap_alloc((long)(length + 1u));
                    if (copy != NULL) memcpy(copy, name, length + 1u);
                    names[found] = copy;
                }
                ++found;
            }
        }
    }
#undef SCENE_SCAN_STDCALL
    return found;
}

// FUNCTION: LEGORR 0x00475c00
/* DDiSceneAnimationCallbacksSet (FUN_00475c00): allocate the four-callback
 * record once, then replace only non-null callback arguments. Native pointer
 * width keeps the record usable in the 64-bit reconstruction. */
void ddiscene_animation_callbacks_set(void* scene_object,
                                      void* callback_a, void* callback_b,
                                      void* callback_c, void* callback_d)
{
    void** callbacks = NULL;
    void* values[4];
    unsigned int i;
    if (scene_object == NULL) return;
    memcpy(&callbacks, (char*)scene_object + 0x0c, sizeof(callbacks));
    if (callbacks == NULL) {
        callbacks = (void**)engine_heap_alloc((long)(4 * sizeof(void*)));
        if (callbacks == NULL) return;
        memset(callbacks, 0, 4 * sizeof(void*));
        memcpy((char*)scene_object + 0x0c, &callbacks, sizeof(callbacks));
    }
    values[0] = callback_a;
    values[1] = callback_b;
    values[2] = callback_c;
    values[3] = callback_d;
    for (i = 0; i < 4; ++i) {
        if (values[i] != NULL) callbacks[i] = values[i];
    }
}

/* DDiScenePayloadRelease (FUN_00475c80): release kind-specific resources,
 * free the payload, and clear the scene's +0x0c slot. */
// FUNCTION: LEGORR 0x00475c80
void ddiscene_payload_release(void* scene_object)
{
    void* payload;
    void* owner;
    uint32_t kind;
    void** fields;
    if (scene_object == NULL) return;
    memcpy(&payload, (char*)scene_object + 0x0c, sizeof(payload));
    if (payload == NULL) return;
    memcpy(&kind, (char*)scene_object + 0x10, sizeof(kind));
    memcpy(&owner, (char*)scene_object + 4, sizeof(owner));
    fields = (void**)payload;
    if (kind == 4u) {
        if (fields[0] != NULL) engine_heap_free(fields[0]);
    }
    else if (kind == 5u && fields[1] != NULL) {
        if (s_ddiscene_payload_release_hooks.interface_release != NULL) {
            s_ddiscene_payload_release_hooks.interface_release(
                fields[1], s_ddiscene_payload_release_hooks.context);
        }
        else {
            ddiscene_payload_interface_release(fields[1]);
        }
    }
    else if (kind == 1u && fields[2] != NULL) {
        if (s_ddiscene_payload_release_hooks.kind1_detach != NULL) {
            s_ddiscene_payload_release_hooks.kind1_detach(
                owner, fields[2], s_ddiscene_payload_release_hooks.context);
        }
        else if (owner != NULL) {
            void** vtable = *(void***)owner;
            if (vtable != NULL && vtable[0xa0 / 4] != NULL) {
                ddiscene_payload_owner_detach_call(
                    owner, fields[2], vtable[0xa0 / 4]);
            }
        }
        if (s_ddiscene_payload_release_hooks.interface_release != NULL) {
            s_ddiscene_payload_release_hooks.interface_release(
                fields[2], s_ddiscene_payload_release_hooks.context);
        }
        else {
            ddiscene_payload_interface_release(fields[2]);
        }
    }
    else if (kind == 8u && fields[3] != NULL &&
             s_ddiscene_payload_release_hooks.kind8_detach != NULL) {
        s_ddiscene_payload_release_hooks.kind8_detach(
            fields[3], owner, s_ddiscene_payload_release_hooks.context);
    }
    engine_heap_free(payload);
    payload = NULL;
    memcpy((char*)scene_object + 0x0c, &payload, sizeof(payload));
}

/* CallbackRecordSetFlag10AndRefresh (FUN_00475e10): treat the primary value
 * as a record pointer, set its flag, then invalidate the property payload. */
// FUNCTION: LEGORR 0x00475e10
void callback_record_set_flag10_and_refresh(void* property)
{
    if (property == NULL) return;
    const uint32_t primary_value = scene_property_primary_value_get(property);
    void* record = (void*)(size_t)primary_value;
    if (record == NULL) return;
    *(uint32_t*)((char*)record + 0x14) |= 0x10u;
    attached_object_data_release_and_clear(property);
}
/* DDiSceneFrameRecordFind (FUN_00475e40): search the selected action-frame
 * collection for the case-insensitive `ActFrame_<name>` record. */
// FUNCTION: LEGORR 0x00475e40
void* ddiscene_frame_record_find(void* action_interface,
                                 const char* action_name, int selector)
{
    char wanted[1024];
    static const char prefix[] = "ActFrame_";
    size_t action_length;
    size_t prefix_length = sizeof(prefix) - 1u;
    unsigned int count;
    unsigned int index;
    DDiSceneFrameRecordFindHooks* const hooks =
        &s_ddiscene_frame_record_find_hooks;
    if (action_interface == NULL || action_name == NULL ||
        hooks->count_get == NULL || hooks->record_get == NULL ||
        hooks->record_name_get == NULL) return NULL;
    action_length = strlen(action_name);
    if (prefix_length + action_length >= sizeof(wanted)) return NULL;
    memcpy(wanted, prefix, prefix_length);
    memcpy(wanted + prefix_length, action_name, action_length + 1u);
    count = hooks->count_get(action_interface, selector, hooks->context);
    if (count > 4096u) count = 4096u;
    for (index = 0; index < count; ++index) {
        void* record = hooks->record_get(action_interface, selector, index,
                                         hooks->context);
        const char* name = record != NULL
            ? hooks->record_name_get(record, hooks->context) : NULL;
        if (name != NULL && strcasecmp_lite(name, wanted) == 0) {
            if (hooks->record_retain != NULL)
                hooks->record_retain(record, hooks->context);
            return record;
        }
    }
    return NULL;
}
// FUNCTION: LEGORR 0x00475f70
/* DDiSceneAnimationHookCreate (FUN_00475f70): allocate the ten-word record
 * once, update only nonzero/non-null arguments, and replace its owned name. */
void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20)
{
    DDiSceneAnimationHookRecord* record;
    DDiSceneAnimationHookRecordHooks* hooks =
        &s_ddiscene_animation_hook_record_hooks;
    if (scene_object == NULL) return;
    if (hooks->record_get != NULL && hooks->record_set != NULL) {
        record = hooks->record_get(scene_object, hooks->context);
    }
    else {
        record = ddiscene_animation_hook_record_get(scene_object);
    }
    if (record == NULL) {
        record = (DDiSceneAnimationHookRecord*)engine_heap_alloc(
            (long)sizeof(DDiSceneAnimationHookRecord));
        if (record == NULL) return;
        memset(record, 0, sizeof(*record));
        record->scene_object = scene_object;
        if (value_0c != NULL) record->value_0c = *value_0c;
        record->value_08 = value_08;
        if (value_10 != NULL) record->value_10 = *value_10;
        if (value_14 != NULL) record->value_14 = *value_14;
        record->value_18 = value_18;
        record->callback = callback;
        if (value_20 != NULL) record->value_20 = *value_20;
        record->value_24 = value_24;
        if (hooks->record_get != NULL && hooks->record_set != NULL) {
            hooks->record_set(scene_object, record, hooks->context);
        }
        else {
            if (!ddiscene_animation_hook_record_set(scene_object, record)) {
                engine_heap_free(record);
                return;
            }
        }
    }
    else {
        if (callback != 0) record->callback = callback;
        if (name != NULL && record->name != NULL) {
            engine_heap_free(record->name);
            record->name = NULL;
        }
        if (value_0c != NULL) record->value_0c = *value_0c;
        if (value_08 != 0) record->value_08 = value_08;
        if (value_10 != NULL) record->value_10 = *value_10;
        if (value_14 != NULL) record->value_14 = *value_14;
        if (value_18 != 0) record->value_18 = value_18;
        if (value_24 != 0) record->value_24 = value_24;
        if (value_20 != NULL) record->value_20 = *value_20;
    }
    if (name != NULL) {
        const size_t length = strlen(name);
        record->name = (char*)engine_heap_alloc((long)(length + 1u));
        if (record->name != NULL) memcpy(record->name, name, length + 1u);
    }
}

/* AttachedObjectDataReleaseAndClear (FUN_004760e0): release the optional
 * record-owned buffer and record, then clear the interface's value slot. */
// FUNCTION: LEGORR 0x004760e0
void attached_object_data_release_and_clear(void* property)
{
    void* record = scene_property_value_record_get(property);
    void* buffer = NULL;
    if (record != NULL) {
        memcpy(&buffer, (char*)record + 4, sizeof(buffer));
        if (buffer != NULL) engine_heap_free(buffer);
        engine_heap_free(record);
    }
    if (property != NULL) {
        ((ScenePropertyValueInterface*)property)->value_record_set(NULL);
    }
}

/* ScenePropertyPrimaryValueGet (FUN_00476120): first dword in the value
 * record returned by the object's virtual callback. */
// FUNCTION: LEGORR 0x00476120
uint32_t scene_property_primary_value_get(void* property)
{
    void** vtable = *(void***)property;
    typedef void* (VIEW_COM_CALL *ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(uint32_t*)record;
    }
    return 0;
}

/* ScenePropertySecondaryValueGet (FUN_00476140): second dword in the same
 * callback result. */
// FUNCTION: LEGORR 0x00476140
uint32_t scene_property_secondary_value_get(void* property)
{
    void** vtable = *(void***)property;
    typedef void* (VIEW_COM_CALL *ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(uint32_t*)((char*)record + 4);
    }
    return 0;
}
/* DDiSceneActionRecordField1CGet (FUN_00476160). */
// FUNCTION: LEGORR 0x00476160
uint32_t ddiscene_action_record_field_1c_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(uint32_t*)((char*)record + 0x1c) : 0u;
}

/* DDiSceneActionRecordField0CGet (FUN_00476180). */
// FUNCTION: LEGORR 0x00476180
uint32_t ddiscene_action_record_field_0c_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(uint32_t*)((char*)record + 0x0c) : 0u;
}
/* DDiSceneAnimationDurationGet (FUN_004761a0). */
// FUNCTION: LEGORR 0x004761a0
float ddiscene_animation_duration_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(float*)((char*)record + 0x10) : 0.0f;
}

/* ScenePropertyFloatAt14Get (FUN_004761c0): read the float at +0x14 in the
 * property record returned by the interface's slot +0x1c. */
// FUNCTION: LEGORR 0x004761c0
float scene_property_float_at14_get(void* property)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    void** vtable = *(void***)property;
    typedef void* (VIEW_COM_CALL *ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(float*)((char*)record + 0x14);
    }
    return 0.0f;
#elif defined(__i386__)
    void** vtable = *(void***)property;
    typedef void* (VIEW_COM_CALL *ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(float*)((char*)record + 0x14);
    }
    return 0.0f;
#else
    void* record = scene_property_value_record_get(property);
    return record != NULL ? *(float*)((char*)record + 0x14) : 0.0f;
#endif
}
/* DDiSceneAnimationStateGet (FUN_004761e0). */
// FUNCTION: LEGORR 0x004761e0
uint32_t ddiscene_animation_state_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(uint32_t*)((char*)record + 0x18) : 0u;
}

/* DDiSceneAnimationLastFrameGet (FUN_00476200). */
// FUNCTION: LEGORR 0x00476200
uint32_t ddiscene_animation_last_frame_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(uint32_t*)((char*)record + 0x20) : 0u;
}
/* RenderInterfacePairUpdate (FUN_00476220): the retail x86 path dispatches
 * the interface methods directly. Other hosts retain the callback override
 * and the checked vtable helper. */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x00476220
__declspec(naked) void render_interface_pair_update(void* first, void* second)
{
    __asm {
        sub esp, 40h
        lea ecx, dword ptr [esp + 48h]
        push esi
        mov esi, dword ptr [esp + 4ch]
        push ecx
        push esi
        mov eax, dword ptr [esi]
        call dword ptr [eax + 5ch]
        mov ecx, dword ptr [esp + 4ch]
        mov edx, dword ptr [esi]
        lea eax, dword ptr [esp + 4]
        push eax
        push ecx
        push esi
        call dword ptr [edx + 74h]
        mov eax, dword ptr [esp + 4ch]
        test eax, eax
        je render_interface_pair_update_release_done
        mov edx, dword ptr [eax]
        push eax
        call dword ptr [edx + 8]
    render_interface_pair_update_release_done:
        mov eax, dword ptr [esp + 48h]
        push esi
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 2ch]
        mov edx, dword ptr [esi]
        lea eax, dword ptr [esp + 4]
        push eax
        push 0
        push esi
        call dword ptr [edx + 38h]
        pop esi
        add esp, 40h
        ret
    }
}
#else
void render_interface_pair_update(void* first, void* second)
{
    if (s_render_interface_pair_hooks.pair_update != NULL) {
        s_render_interface_pair_hooks.pair_update(
            first, second, s_render_interface_pair_hooks.context);
    }
    else {
        ddiscene_action_channel_apply(first, second);
    }
}
#endif
// FUNCTION: LEGORR 0x00476270
/* SceneObjectSetFormattedString (FUN_00476270): format twice using the same
 * arguments, allocate an exact-size owned string, set the scene interface
 * string, then bind it to the animation path in the recovered order. */
void scene_object_set_formatted_string(void* scene_object,
                                       const char* format, ...)
{
    char local[1024];
    char* owned;
    size_t length;
    va_list arguments;
    int string_was_set;
    if (scene_object == NULL || format == NULL) return;
    va_start(arguments, format);
    length = (size_t)formatted_text_buffer_build(
        local, sizeof(local), format, arguments);
    va_end(arguments);
    owned = (char*)engine_heap_alloc((long)(length + 1));
    if (owned == NULL) return;
    va_start(arguments, format);
    formatted_text_buffer_build(owned, length + 1, format, arguments);
    va_end(arguments);
    string_was_set = 0;
    if (s_scene_formatted_string_hooks.string_set != NULL) {
        s_scene_formatted_string_hooks.string_set(
            scene_object, owned, s_scene_formatted_string_hooks.context);
        string_was_set = 1;
    }
    else {
        void** vtable = *static_cast<void***>(scene_object);
        typedef void (VIEW_COM_CALL *SceneStringSetCall)(void*, const char*);
        if (vtable != NULL && vtable[0x20 / 4] != NULL) {
            SceneStringSetCall set_string =
                reinterpret_cast<SceneStringSetCall>(vtable[0x20 / 4]);
            set_string(scene_object, owned);
            string_was_set = 1;
        }
    }
    if (s_scene_formatted_string_hooks.animation_string_bind != NULL) {
        s_scene_formatted_string_hooks.animation_string_bind(
            scene_object, owned, s_scene_formatted_string_hooks.context);
    }
    if (!string_was_set) {
        engine_heap_free(owned);
    }
}

// FUNCTION: LEGORR 0x004762f0
void scene_property_owned_buffer_release(void* property)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    void* owned_buffer = (void*)scene_property_value_at08_get(property);
    if (owned_buffer != NULL) {
        engine_heap_free(owned_buffer);
    }
#elif defined(__i386__)
    void* owned_buffer = (void*)scene_property_value_at08_get(property);
    if (owned_buffer != NULL) {
        engine_heap_free(owned_buffer);
    }
#else
    void* record;
    void* owned_buffer;
    if (property == NULL) {
        return;
    }
    record = ((ResourcePropertyInterface*)property)->property_get();
    owned_buffer = record != NULL ? *(void**)((char*)record + 0x08) : NULL;
    engine_heap_free(owned_buffer);
#endif
}

/* ScenePropertyValueAt08Get (FUN_00476310). */
// FUNCTION: LEGORR 0x00476310
uint32_t scene_property_value_at08_get(void* scene_interface)
{
    void** vtable = *(void***)scene_interface;
    typedef void* (VIEW_COM_CALL *DDiSceneCurrentPropertyGetFn)(void*);
    void* record = ((DDiSceneCurrentPropertyGetFn)vtable[7])(scene_interface);
    return record != NULL ? *(uint32_t*)((char*)record + 0x08) : 0u;
}

/* Retail ResourceTreeCallbackWalk (FUN_00476330) operates on Direct3DRM
 * interfaces, not ResourceTreeNode. Keep this target entry separate from the
 * portable test/runtime tree API above. Its traversal mirrors the COM slots
 * and reference ownership visible in the retail body. */
// FUNCTION: LEGORR 0x00476330
int resource_tree_callback_walk_target(void* object, int depth,
                                       ResourceTreeCallback callback,
                                       void* context)
{
    void** object_vtable;
    ResourceTreeGetChildrenFn get_children;
    void* child_array;
    void** array_vtable;
    ResourceTreeArraySizeFn get_size;
    uint32_t child_count;
    uint32_t index;
    int found = 0;
    if (callback(object, context) != 0) return 1;

    object_vtable = *(void***)object;
    get_children = (ResourceTreeGetChildrenFn)object_vtable[0x4c / 4];
    child_array = NULL;
    if (get_children(object, &child_array) != 0) return 0;

    array_vtable = *(void***)child_array;
    get_size = (ResourceTreeArraySizeFn)array_vtable[0x0c / 4];
    child_count = get_size(child_array);
    for (index = 0u; index < child_count; ++index) {
        void* child;
        void* child_interface;
        void** child_vtable;
        void** current_array_vtable;
        ResourceTreeArrayElementFn get_element;
        ResourceTreeQueryInterfaceFn query_interface;
        ResourceTreeReferenceFn release_child;
        ResourceTreeReferenceFn release_interface;

        child = NULL;
        child_interface = NULL;
        current_array_vtable = *(void***)child_array;
        get_element = (ResourceTreeArrayElementFn)
            current_array_vtable[0x10 / 4];
        (void)get_element(child_array, index, &child);

        child_vtable = *(void***)child;
        query_interface = (ResourceTreeQueryInterfaceFn)child_vtable[0];
        (void)query_interface(child, s_resource_tree_child_iid,
                              &child_interface);
        release_child = (ResourceTreeReferenceFn)child_vtable[2];
        (void)release_child(child);

        if (resource_tree_callback_walk_target(child_interface, depth + 1,
                                               callback, context) != 0) {
            void** child_interface_vtable = *(void***)child_interface;
            release_interface = (ResourceTreeReferenceFn)
                child_interface_vtable[2];
            (void)release_interface(child_interface);
            found = 1;
            break;
        }
        {
            void** child_interface_vtable = *(void***)child_interface;
            release_interface = (ResourceTreeReferenceFn)
                child_interface_vtable[2];
            (void)release_interface(child_interface);
        }
    }
    {
        void** current_array_vtable = *(void***)child_array;
        ResourceTreeReferenceFn release_array =
            (ResourceTreeReferenceFn)current_array_vtable[2];
        (void)release_array(child_array);
    }
    return found;
}

/* ResourceNameMatchCallback @ 0x00476410. The retail callback receives a
 * Direct3DRM object and the seven-word query record built by
 * ResourceNameRegisterOrQuery. Name retrieval uses the object's +0x24 slot;
 * the portable ResourceTreeNode predicate above remains for portable callers. */
// FUNCTION: LEGORR 0x00476410
int resource_name_match_callback_target(void* object, void* context)
{
    ResourceNameMatchCallbackContext* query =
        (ResourceNameMatchCallbackContext*)context;
    void** object_vtable;
    ResourceTreeGetNameFn get_name;
    uint32_t object_name_bytes = 0u;
    char* object_name;
    uint32_t mode;
    int equal;

    object_vtable = *(void***)object;
    get_name = (ResourceTreeGetNameFn)object_vtable[0x24 / 4];
    if (get_name(object, &object_name_bytes, NULL) != 0u ||
        object_name_bytes == 0u ||
        object_name_bytes - 1u != query->name_length) {
        return 0;
    }
    object_name = (char*)engine_heap_alloc((long)object_name_bytes);
    if (object_name == NULL) return 0;
    object_name[0] = '\0';
    if (get_name(object, &object_name_bytes, object_name) != 0u) {
        engine_heap_free(object_name);
        return 0;
    }

    mode = (uint32_t)query->mode;
    if (mode == 0u || mode == 2u) query->matched_object = NULL;
    equal = resource_name_compare_bytes(object_name, query->name,
                                        query->case_sensitive != 0u);
    if (equal != 0) {
        if (mode == 0u) {
            query->matched_object = object;
        } else if (mode == 1u) {
            ++query->match_count;
        } else if (mode == 2u) {
            ++query->match_count;
            if (query->match_count == query->selected_index + 1u) {
                query->matched_object = object;
            }
        }
    }
    engine_heap_free(object_name);
    return query->matched_object != NULL;
}

// FUNCTION: LEGORR 0x00476770
int ddiscene_resource_name_resolve(const char* name)
{
    static const char prefix[] = "DDiScene_";
    const char* suffix;
    int resolved;
    resolved = 0;
    suffix = strstr(name, prefix);
    if (suffix != NULL) {
        suffix += sizeof(prefix) - 1;
        while (*suffix != '\0' &&
               !crt_character_classify_mask((unsigned char)*suffix, 4)) {
            ++suffix;
        }
        resolved = atoi(suffix);
    }
    return (resolved != 0) ? resolved : 1;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x004767e0
int ledger_stub_004767e0_DDiSceneResourceExists(const char* name,
                                                 void* scene)
{
    Resource* file;
    int32_t file_size;
    int exists;
    if (name == NULL || scene == NULL) return 0;
    /* FUN_004767e0 reads the original path, then asks the supplied scene
     * interface to parse that in-memory file. Resource::data stays alive
     * through the query and is released on both success and parse failure. */
    file = resource_open(name, RESOURCE_AUTO);
    if (file == NULL) return 0;
    file_size = resource_read(file);
    if (file_size < 0) {
        resource_free(file);
        return 0;
    }
    exists = 0;
    if (s_ddiscene_resource_exists_hook != NULL) {
        exists = s_ddiscene_resource_exists_hook(
            file->data, (uint32_t)file_size, name, scene,
            s_ddiscene_resource_hook_context);
    }
    resource_free(file);
    return exists;
}

// DDiSceneResourceOpen
// FUNCTION: LEGORR 0x00476850
void* ddiscene_resource_open(void* resource_handle, const void* file_data,
                             const char* path, void* scene_interface,
                             uint32_t optional_flag)
{
    DdiSceneMeshLoadContext load_context;
    load_context.path = path;
    load_context.optional_flag = optional_flag != 0u ? 1u : 0u;
    load_context.resource_handle = resource_handle;
    load_context.file_data = file_data;
    /* The target creates an IDirect3DRMMeshBuilder through global
     * DAT_00505914 (+0x18), calls Load (+0x2c, source record, NULL, 2,
     * LAB_00476900, source record), gets the mesh (+0xc0), releases the
     * builder, then attaches through scene-interface +0x48. The portable
     * runtime has no IDirect3DRM3 owner or texture callback interface, so
     * keep that operation behind the renderer hook while preserving the
     * exact callback context and optional-flag normalization. */
    if (s_ddiscene_resource_open_hook == NULL) return NULL;
    return s_ddiscene_resource_open_hook(&load_context, scene_interface,
                                         s_ddiscene_resource_hook_context);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00476bf0
void surface_pixel_rows_flip_in_place(void* surface)
{
    SurfacePixelInfo* info =
        ((SurfacePixelInfoOwner*)surface)->get_pixel_info();
    if (info != NULL) {
        const int row_bits = info->bits_per_pixel * info->width;
        const uint32_t row_bytes = (uint32_t)(
            (row_bits + ((row_bits >> 31) & 7)) >> 3);
        unsigned char* scratch = (unsigned char*)engine_heap_alloc(row_bytes);
        if (scratch != NULL) {
            unsigned char* top = (unsigned char*)info->pixels;
            unsigned char* bottom = top
                + (ptrdiff_t)(info->height - 1) * info->row_stride;
            for (int row = 0; row < info->height / 2; ++row) {
                memcpy(scratch, bottom, row_bytes);
                memcpy(bottom, top, row_bytes);
                memcpy(top, scratch, row_bytes);
                top += info->row_stride;
                bottom -= info->row_stride;
            }
            engine_heap_free(scratch);
        }
    }
}

// FUNCTION: LEGORR 0x00476d10
void render_resource_pair_release_and_clear(void** pair)
{
    void* property;
    if (pair == NULL) {
        return;
    }
    property = pair[1] != NULL
        ? ((ResourcePropertyInterface*)pair[1])->property_get() : NULL;
    engine_heap_free(property);
    engine_heap_free(pair[0]);
    pair[0] = NULL;
    pair[1] = NULL;
}

/* FUN_00476220, reconstructed from the retail call bytes. The slot offsets
 * and argument stack shapes are exact; the original method names are not
 * established. */
static void ddiscene_action_channel_apply(void* channel, void* action_record)
{
    void** record_vtable;
    void** channel_vtable;
    void* temporary = NULL;
    if (channel == NULL || action_record == NULL) return;
    record_vtable = *(void***)action_record;
    channel_vtable = *(void***)channel;
    if (record_vtable == NULL || channel_vtable == NULL) return;

    /* Retail pushes (channel, record) to +0x5c, then (record, &out) to
     * +0x74. Both slot indices and argument order are from 0x476220 bytes. */
    if (record_vtable[0x5c / 4] != NULL) {
        DdiSceneActionQuery query =
            (DdiSceneActionQuery)record_vtable[0x5c / 4];
        (void)query(action_record, channel);
    }
    if (record_vtable[0x74 / 4] != NULL) {
        DdiSceneActionCreate create =
            (DdiSceneActionCreate)record_vtable[0x74 / 4];
        (void)create(action_record, action_record, &temporary);
    }
    if (temporary != NULL) {
        void** temporary_vtable = *(void***)temporary;
        if (temporary_vtable != NULL && temporary_vtable[2] != NULL) {
            DdiSceneActionRelease release =
                (DdiSceneActionRelease)temporary_vtable[2];
            release(temporary);
        }
    }
    if (channel_vtable[0x2c / 4] != NULL) {
        DdiSceneActionBind bind =
            (DdiSceneActionBind)channel_vtable[0x2c / 4];
        bind(channel, action_record);
    }
    if (record_vtable[0x38 / 4] != NULL) {
        void* output = NULL;
        DdiSceneActionBindOutput bind_output =
            (DdiSceneActionBindOutput)record_vtable[0x38 / 4];
        bind_output(action_record, 0u, &output);
    }
}

static int scene_act_frame_name_match(const char* name)
{
    static const char prefix[] = "ActFrame_";
    unsigned int i;
    if (name == NULL) return 0;
    for (i = 0; i < 8; ++i) {
        int a = (unsigned char)name[i];
        int b = (unsigned char)prefix[i];
        if (a == 0) return 0;
        if (a >= 'a' && a <= 'z') a -= ('a' - 'A');
        if (b >= 'a' && b <= 'z') b -= ('a' - 'A');
        if (a != b) return 0;
    }
    return 1;
}

static void ddiscene_destroy_action_frame_records(void* scene_object)
{
    unsigned int count;
    void** records;
    int* frame_indices;
    unsigned int index;

    count = scene_action_record_act_frame_scan(scene_object, NULL, NULL,
                                                NULL);
    if (count == 0u) return;
    records = (void**)engine_heap_alloc((long)(count * sizeof(void*)));
    if (records == NULL) return;
    frame_indices = (int*)engine_heap_alloc((long)(count * sizeof(int)));
    if (frame_indices == NULL) {
        engine_heap_free(records);
        return;
    }
    memset(records, 0, count * sizeof(void*));
    memset(frame_indices, 0, count * sizeof(int));
    count = scene_action_record_act_frame_scan(scene_object, records,
                                                frame_indices, NULL);
    for (index = 0u; index < count; ++index) {
        if (records[index] == NULL) continue;
        scene_property_owned_buffer_release(records[index]);
        attached_object_data_release_and_clear(records[index]);
        ddiscene_payload_interface_release(records[index]);
    }
    engine_heap_free(frame_indices);
    engine_heap_free(records);
}

/* Kind-one objects keep a second, renderer-owned list of child interfaces in
 * the payload's +8 interface.  FUN_00472a50 walks that list, detaches each
 * child from the scene owner, releases it, then frees the item array and its
 * two-word container.  The kind-one payload release below separately handles
 * the payload's +8 retained interface, as FUN_00475c80 does. */
static void ddiscene_destroy_kind1_child_interfaces(void* scene_object)
{
    unsigned char* scene = (unsigned char*)scene_object;
    void* owner;
    void* payload;
    uint32_t nested_owner;
    void* list_interface;
    void** vtable;
    void* container;
    void** items;
    uint32_t count;
    uint32_t index;

    /* The scene/payload record is the retail 32-bit layout. A wider host
     * needs the renderer destroy hook's sidecar view instead of raw offsets. */
    if (sizeof(void*) != 4u) return;
    memcpy(&owner, scene + 4, sizeof(owner));
    memcpy(&payload, scene + 0x0c, sizeof(payload));
    if (owner == NULL || payload == NULL) return;
    memcpy(&nested_owner, (unsigned char*)payload + 0x0c,
           sizeof(nested_owner));
    if (nested_owner != 0u) return;
    memcpy(&list_interface, (unsigned char*)payload + 8,
           sizeof(list_interface));
    if (list_interface == NULL) return;
    vtable = *(void***)list_interface;
    if (vtable == NULL || vtable[0x1c / 4] == NULL) return;
#if defined(_MSC_VER) && defined(_M_IX86)
    {
        void* result;
        void* method = vtable[0x1c / 4];
        __asm {
            mov ecx, list_interface
            mov eax, method
            call eax
            mov result, eax
        }
        container = result;
    }
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
    {
        typedef void* (__attribute__((thiscall)) *ChildListGet)(void*);
        container = ((ChildListGet)vtable[0x1c / 4])(
            list_interface);
    }
#else
    {
        typedef void* (*ChildListGet)(void*);
        container = ((ChildListGet)vtable[0x1c / 4])(
            list_interface);
    }
#endif
    if (container == NULL) return;

    memcpy(&items, container, sizeof(items));
    memcpy(&count, (unsigned char*)container + 4, sizeof(count));
    vtable = *(void***)owner;
    if (items != NULL && vtable != NULL &&
        vtable[0xa0 / 4] != NULL) {
        for (index = 0; index < count; ++index) {
            void* child = items[index];
            if (child == NULL) continue;
            ddiscene_payload_owner_detach_call(
                owner, child, vtable[0xa0 / 4]);
            ddiscene_payload_interface_release(child);
        }
    }
    if (items != NULL) engine_heap_free(items);
    engine_heap_free(container);
}

static SceneLightInterface* scene_light_interface(void* record)
{
    void* const light = *(void**)((char*)record + 0x0c);
    return (SceneLightInterface*)*(void**)((char*)light + 4);
}

static DDiSceneObjectPoseInterface* ddiscene_pose_interface_resolve(
    void* object, void* alternate_frame, void** normalized_frame)
{
    const void* normalized_object = NULL;
    const void* resolved_frame = NULL;
    void* normalized_object_mutable = NULL;
    if (object == NULL || normalized_frame == NULL) return NULL;
    scene_object_arguments_normalize((const void**)object,
        (const void**)alternate_frame,
        &normalized_object_mutable, &resolved_frame);
    normalized_object = normalized_object_mutable;
    if (normalized_object == NULL) return NULL;
    *normalized_frame = (void*)resolved_frame;
    return (DDiSceneObjectPoseInterface*)normalized_object;
}

static void* scene_property_value_record_get(void* property)
{
    void** vtable = NULL;
    if (property == NULL) {
        return NULL;
    }
    __builtin_memcpy(&vtable, property, sizeof vtable);
    if (vtable == NULL) {
        return NULL;
    }
    if (vtable[7] == NULL) {
        return NULL;
    }
    return ((ScenePropertyValueInterface*)property)->value_record_get();
}

static int resource_name_compare_bytes(const char* left, const char* right,
                                       int case_sensitive)
{
    while (*left != '\0' && *right != '\0') {
        unsigned char a = (unsigned char)*left;
        unsigned char b = (unsigned char)*right;
        if (case_sensitive == 0) {
            if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
            if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        }
        if (a != b) return 0;
        ++left;
        ++right;
    }
    return *left == *right;
}

namespace lego {
namespace {
void scene_object_allocation_failed(DDiSceneObjectRecord* record)
{
    /* FUN_00472950 sends a partially initialized record through the normal
     * mode-zero destructor.  The registered adapter owns the full renderer
     * cleanup and pool transition when present; the local fallback can only
     * release channels whose ownership was established here. */
    if (s_object_destroy != 0)
        s_object_destroy(record, 0);
    else
        scene_object_partial_release(record);
}
} // namespace
} // namespace lego

namespace lego {
static int interface_forward_sync_items_reserve(uint32_t count)
{
    uint32_t capacity;
    void** items;
    size_t bytes;
    if (count <= s_interface_forward_sync_items_capacity) return 1;
    capacity = s_interface_forward_sync_items_capacity != 0u
        ? s_interface_forward_sync_items_capacity : 4u;
    while (capacity < count) {
        if (capacity > 0x7fffffffu / 2u) {
            capacity = count;
            break;
        }
        capacity *= 2u;
    }
    if ((size_t)capacity > (size_t)-1 / sizeof(void*)) return 0;
    bytes = (size_t)capacity * sizeof(void*);
    items = (void**)malloc(bytes);
    if (items == NULL) return 0;
    if (s_interface_forward_sync_items != NULL) {
        memcpy(items, s_interface_forward_sync_items,
               (size_t)s_interface_forward_sync_items_capacity *
                   sizeof(void*));
        free(s_interface_forward_sync_items);
    }
    s_interface_forward_sync_items = items;
    s_interface_forward_sync_items_capacity = capacity;
    return 1;
}

static void interface_array_forward_sync_slot80(void* object,
                                                 uint32_t* count,
                                                 void** items)
{
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*, void*);
    if (object == NULL || count == NULL) return;
    if (s_interface_array_forward_sync_hooks.enumerate != NULL) {
        s_interface_array_forward_sync_hooks.enumerate(
            s_interface_array_forward_sync_hooks.context, object, count,
            items);
        return;
    }
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[0x80 / 4] == NULL) return;
    ((Method)vtable[0x80 / 4])(object, count, items);
}

static void interface_array_forward_sync_slot(void* object,
                                               unsigned int byte_offset,
                                               void* value)
{
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*);
    if (object == NULL) return;
    if (s_interface_array_forward_sync_hooks.set_value != NULL) {
        s_interface_array_forward_sync_hooks.set_value(
            s_interface_array_forward_sync_hooks.context, object,
            byte_offset, value);
        return;
    }
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[byte_offset / 4] == NULL) return;
    ((Method)vtable[byte_offset / 4])(object, value);
}
} // namespace lego
