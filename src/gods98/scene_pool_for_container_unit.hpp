// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what container_unit uses from scene_pool.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SCENE_POOL_FOR_CONTAINER_UNIT_HPP
#define LRR_GODS98_SCENE_POOL_FOR_CONTAINER_UNIT_HPP

namespace lego {
struct DDiSceneObjectRecord {
    void* channels[3];
    void* secondary_handle;
    uint32_t kind;
    uint32_t flags;
    void* owned_name;
    uint32_t reserved[3];
    DDiSceneObjectRecord* next;
};

enum { DDI_SCENE_POOL_BANKS = 20 };
} // namespace lego

namespace lego {
extern DDiSceneObjectRecord* s_scene_object_blocks[DDI_SCENE_POOL_BANKS];
extern uint32_t s_scene_object_level_count;
extern DDiSceneObjectRecord* s_scene_object_free_head;
extern DDiSceneChannelCreateFn s_channel_create;
extern DDiSceneObjectDestroyFn s_object_destroy;
extern DDiSceneAuxiliaryReleaseFn s_auxiliary_release;
extern void** s_auxiliary_allocations;
extern uint32_t s_auxiliary_count;
extern void* s_shared_directory;
void scene_object_return_to_pool(DDiSceneObjectRecord* record);
void scene_object_partial_release(DDiSceneObjectRecord* record);
} // namespace lego

#endif /* LRR_GODS98_SCENE_POOL_FOR_CONTAINER_UNIT_HPP */
