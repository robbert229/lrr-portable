// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GODS98_SCENE_POOL_HPP
#define LEGORRR_GODS98_SCENE_POOL_HPP

#include <stdint.h>

namespace lego {

struct DDiSceneObjectDescriptor {
    const char* channel_names[3];
    uint32_t kind;
    uint32_t flags;
};

typedef int (*DDiSceneChannelCreateFn)(const char* name,
                                       void* object_record,
                                       uint32_t channel_index,
                                       void** channel_out);
typedef void (*DDiSceneChannelReleaseFn)(void* channel);
typedef void (*DDiSceneObjectDestroyFn)(void* object_record,
                                        int shutdown_mode);
typedef void (*DDiSceneAuxiliaryReleaseFn)(void* allocation);

void ddiscene_object_pool_hooks_set(
    DDiSceneChannelCreateFn channel_create,
    DDiSceneChannelReleaseFn channel_release,
    DDiSceneObjectDestroyFn object_destroy,
    DDiSceneAuxiliaryReleaseFn auxiliary_release);
void ddiscene_object_pool_auxiliary_set(void** allocations, uint32_t count,
                                        void* shared_directory);
void ddiscene_object_array_grow(void);
void* ddiscene_object_allocate(const DDiSceneObjectDescriptor* descriptor);
void ddiscene_object_pool_shutdown(void); /* FUN_00472800 */

} // namespace lego

#endif
