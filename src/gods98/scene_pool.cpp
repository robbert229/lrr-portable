// SPDX-License-Identifier: LGPL-3.0-or-later
#include "scene_pool.hpp"

#include "heap.hpp"
#include "view.hpp"

#include <string.h>

namespace lego {

namespace {

struct DDiSceneObjectRecord {
    void* channels[3];
    void* secondary_handle;
    uint32_t kind;
    uint32_t flags;
    void* owned_name;
    uint32_t reserved[3];
    DDiSceneObjectRecord* next;
};
typedef char DDiSceneObjectRecordSizeCheck[
    sizeof(void*) != 4u || sizeof(DDiSceneObjectRecord) == 0x2cu ? 1 : -1];

enum { DDI_SCENE_POOL_BANKS = 20 };

DDiSceneObjectRecord* s_scene_object_blocks[DDI_SCENE_POOL_BANKS];
uint32_t s_scene_object_level_count;
DDiSceneObjectRecord* s_scene_object_free_head;
DDiSceneChannelCreateFn s_channel_create;
DDiSceneChannelReleaseFn s_channel_release;
DDiSceneObjectDestroyFn s_object_destroy;
DDiSceneAuxiliaryReleaseFn s_auxiliary_release;
void** s_auxiliary_allocations;
uint32_t s_auxiliary_count;
void* s_shared_directory;

void scene_object_return_to_pool(DDiSceneObjectRecord* record)
{
    if (record == 0) return;
    if (record->owned_name != 0) {
        engine_heap_free(record->owned_name);
        record->owned_name = 0;
    }
    record->next = s_scene_object_free_head;
    s_scene_object_free_head = record;
}

void scene_object_partial_release(DDiSceneObjectRecord* record)
{
    unsigned int i;
    if (record == 0) return;
    for (i = 0; i < 3; ++i) {
        if (record->channels[i] != 0 && s_channel_release != 0)
            s_channel_release(record->channels[i]);
        record->channels[i] = 0;
    }
    scene_object_return_to_pool(record);
}

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

void ddiscene_object_record_release(void* record)
{
    scene_object_partial_release((DDiSceneObjectRecord*)record);
}

int ddiscene_object_pool_owns(const void* record)
{
    uint32_t i;
    uintptr_t address = (uintptr_t)record;
    for (i = 0; i < s_scene_object_level_count; ++i) {
        uintptr_t start = (uintptr_t)s_scene_object_blocks[i];
        size_t bytes = (size_t)(1u << i) * sizeof(DDiSceneObjectRecord);
        if (start != 0 && address >= start && address - start < bytes &&
            (address - start) % sizeof(DDiSceneObjectRecord) == 0) return 1;
    }
    return 0;
}

void ddiscene_object_pool_hooks_set(
    DDiSceneChannelCreateFn channel_create,
    DDiSceneChannelReleaseFn channel_release,
    DDiSceneObjectDestroyFn object_destroy,
    DDiSceneAuxiliaryReleaseFn auxiliary_release)
{
    s_channel_create = channel_create;
    s_channel_release = channel_release;
    s_object_destroy = object_destroy;
    s_auxiliary_release = auxiliary_release;
}

void ddiscene_object_pool_auxiliary_set(void** allocations, uint32_t count,
                                        void* shared_directory)
{
    s_auxiliary_allocations = allocations;
    s_auxiliary_count = count;
    s_shared_directory = shared_directory;
}

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
