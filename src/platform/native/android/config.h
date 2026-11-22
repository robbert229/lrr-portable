// SPDX-License-Identifier: LGPL-3.0-or-later
/* Android: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/android/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): OpenGL ES 3, which every
 * Android device since 4.3 can draw with (issue #927). Its name makes
 * entry.cpp create an OpenGL window, without which miniwin lists no OpenGL
 * device. SDL GPU needs Vulkan, which many devices and emulators lack, and
 * upstream lists it on Android only once a window is set up for DirectDraw,
 * after the game lists its devices. When OpenGL ES 3 cannot be set up, the
 * software renderer draws. A device named in LegoRR.ini still wins. */
#define ANDROID_DEFAULT_3D_DEVICE LRR_DEVICE_GLES3

void Android_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_ANDROID_CONFIG_H */
