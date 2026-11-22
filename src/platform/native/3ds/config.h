// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo 3DS: the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/3ds/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_N3DS_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_N3DS_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* Citro3D, the 3DS's own GPU backend: miniwin builds no SDL GPU device
 * here, and the software renderer is far too slow on the ARM11. Until
 * Citro3D draws immediate mode (Miniwin_DeviceDrawsImmediateMode),
 * LRR_ChooseDevice passes over it to the software renderer, which does. */
#define N3DS_DEFAULT_3D_DEVICE "Miniwin driver (Citro3D)"

void N3DS_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_N3DS_CONFIG_H */
