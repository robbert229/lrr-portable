// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo Switch: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/switch/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_NX_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_NX_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): SDL GPU. Miniwin builds no
 * software renderer here, so SDL GPU is the only device that draws the
 * level. */
#define NX_DEFAULT_3D_DEVICE LRR_DEVICE_SDLGPU

void NX_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_NX_CONFIG_H */
