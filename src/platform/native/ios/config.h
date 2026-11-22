// SPDX-License-Identifier: LGPL-3.0-or-later
/* iOS: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/ios/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_IOS_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_IOS_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): SDL GPU, through Metal.
 * The software renderer works but is too slow on the device's CPU. */
#define IOS_DEFAULT_3D_DEVICE LRR_DEVICE_SDLGPU

void IOS_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_IOS_CONFIG_H */
