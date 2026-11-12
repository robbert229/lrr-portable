// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GODS98_SCENE_POOL_HPP
#define LEGORRR_GODS98_SCENE_POOL_HPP

#include <stdint.h>

namespace lego {

struct DDiSceneObjectDescriptor {
    /* An existing scene wrapper supplies the visible and hidden parents.
     * These are interfaces, not resource/channel names (retail +0 and +8). */
    void* channels[3];
};

typedef int (*DDiSceneChannelCreateFn)(void* parent,
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
/* Return a live record after its kind-specific payload has been released. */
void ddiscene_object_record_release(void* record);
int ddiscene_object_pool_owns(const void* record);
extern "C" void ddiscene_object_pool_shutdown(void); /* FUN_00472800 */

} // namespace lego

#endif
