// SPDX-License-Identifier: LGPL-3.0-or-later
/* Android: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/android/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): SDL GPU, through Vulkan.
 * The software renderer works but is too slow on the device's CPU. */
#define ANDROID_DEFAULT_3D_DEVICE LRR_DEVICE_SDLGPU

void Android_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H */
