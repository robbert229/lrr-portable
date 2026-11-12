// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 viewport getters that units rebuilt as C call.
 *
 * C boundary header (ADR 0005) for gods98/scene_resources.hpp, whose
 * matching declarations carry extern "C"; src/game/test_c_boundaries.cpp
 * includes both. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_SCENE_RESOURCES_H
#define LEGORRR_GODS98_SCENE_RESOURCES_H


#ifdef __cplusplus
extern "C" {
#endif

/* Viewport_GetBackClip (FUN_00476fd0) and Viewport_GetFrontClip
 * (FUN_00476ff0). */
float resource_interface_call_slot8c(void* wrapper);
float resource_interface_call_slot90(void* wrapper);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SCENE_RESOURCES_H */
