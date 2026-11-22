// SPDX-License-Identifier: LGPL-3.0-or-later
/* Android: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/android/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): the software renderer,
 * which draws on every device and emulator (issue #927). SDL GPU needs
 * Vulkan, which many devices and emulators lack or implement badly, and
 * the OpenGL ES 3 backend does not draw the level yet (#925). A device
 * named in LegoRR.ini still wins. */
#define ANDROID_DEFAULT_3D_DEVICE LRR_DEVICE_SOFTWARE

void Android_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H */
