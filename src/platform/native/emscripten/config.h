// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web (Emscripten): the configuration defaults (platform/native/config.cpp), as
 * isle-portable's ISLE/emscripten/config.h. */
#ifndef LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_CONFIG_H

#include <dictionary.h>

#include "platform/native/config.h"

/* The default 3D device (LRR_Default3DDevice): the software renderer. SDL
 * GPU has no WebGPU path here. The OpenGL ES 3 backend draws the level, but
 * a WebGL canvas shows a frame only when its thread returns to the browser,
 * which WinMain's worker never does (docs/building.md), so the web build
 * leaves it out. */
#define EMSCRIPTEN_DEFAULT_3D_DEVICE LRR_DEVICE_SOFTWARE

void Emscripten_SetupDefaultConfigOverrides(dictionary* p_dictionary);

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_CONFIG_H */
