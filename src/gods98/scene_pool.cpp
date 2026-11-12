// SPDX-License-Identifier: LGPL-3.0-or-later
#include "scene_pool.hpp"

#include "heap.hpp"
#include "view.hpp"

#include <string.h>
#include "scene_pool_for_container_unit.hpp"

namespace lego {

namespace {

typedef char DDiSceneObjectRecordSizeCheck[
    sizeof(void*) != 4u || sizeof(DDiSceneObjectRecord) == 0x2cu ? 1 : -1];

} // namespace
DDiSceneObjectRecord* s_scene_object_blocks[DDI_SCENE_POOL_BANKS];
uint32_t s_scene_object_level_count;
DDiSceneObjectRecord* s_scene_object_free_head;
DDiSceneChannelCreateFn s_channel_create;
namespace {
DDiSceneChannelReleaseFn s_channel_release;
} // namespace
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

} // namespace lego
