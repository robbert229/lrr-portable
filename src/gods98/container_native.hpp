// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_CONTAINER_NATIVE_HPP
#define LEGORRR_CONTAINER_NATIVE_HPP

#include <stdint.h>

#include "lwsc_scene.hpp"

#if defined(LEGORRR_BACKEND_MINIWIN)

/* Native (miniwin) Containers for LightWave activity objects (.ae "ACT"
 * files): the minifigure and rock-monster scenes that the Creature unit
 * drives. Each one owns an LWSC rendering instance on miniwin D3DRM frames
 * (lwsc_miniwin.hpp); its root frame is the Container's master frame.
 *
 * The Creature unit (game/animation_object.c, GAME-004) reaches them
 * through the native-only forwarders in container_route.cpp, which its C
 * boundary header routes its Container calls to (container_route.h); the
 * gods98 Container functions themselves are unchanged. A native
 * Container keeps the retail head size, so the owner word stays at +0x20
 * for scene_object_owner_get/set and the scene-end callback.
 *
 * Main thread only. */

/* Container_Load(root, path, "ACT", TRUE): open `<path>\<name>.ae` under
 * the "Lego*" root and build its scene. `name_keys` are the .ae keys whose
 * values container_native_configured_name returns. */
void* container_native_load_act(const char* entity_path, uint32_t feature,
                                const char* const* name_keys,
                                unsigned int name_key_count);
int container_native_owns(const void* cont);
const char* container_native_configured_name(const void* cont,
                                             const char* key);

void* container_native_clone(void* cont);              /* Container_Clone */
void container_native_remove(void* cont);              /* Container_Remove */
void container_native_hidden_set(void* cont, int hidden);
int container_native_hidden(const void* cont);

/* Container_SetActivity: nonzero when the activity exists. */
uint32_t container_native_activity_set(void* cont, const char* activity);
/* Container_SetAnimationTime / Container_ForwardAnimation: both return how
 * far the new time overran the activity's last frame (0 when it did not).
 * Frame-time units, as the .lws files author them. */
float container_native_animation_time_set(void* cont, float time);
float container_native_animation_forward(void* cont, float delta);
uint32_t container_native_animation_frames(void* cont);
float container_native_animation_time(void* cont);
/* Container_GetTransCoef of the current activity. */
float container_native_trans_coef(void* cont);

/* Container_SetPosition / Container_SetOrientation relative to the scene
 * root, with D3DRM semantics (direction is the frame's z axis, up its y). */
int container_native_position_set(void* cont, float x, float y, float z);
int container_native_orientation_set(void* cont, float dx, float dy,
                                     float dz, float ux, float uy, float uz);
/* World position of a native Container or of a frame it returned. */
int container_native_object_known(const void* object);
int container_native_position_get(const void* object, float xyz[3]);

/* Container_FormatPartName and Container_SearchTree for native Containers:
 * the formatted name is only meaningful to container_native_part_find. */
const char* container_native_part_name_format(void* cont, const char* name,
                                              const int* instance);
void* container_native_part_find(void* cont, const char* formatted_name);

/* Visit the visible meshes of the current activity, with matrices from
 * model space to the Container's root-frame space (the creature's own
 * space, excluding its position and orientation). */
typedef void (*ContainerNativeMeshVisitFn)(void* context, const Model* model,
                                           const float model_to_root[16],
                                           float opacity);
unsigned int container_native_meshes_visit(void* cont,
                                           ContainerNativeMeshVisitFn visit,
                                           void* context);

#endif /* LEGORRR_BACKEND_MINIWIN */
#endif /* LEGORRR_CONTAINER_NATIVE_HPP */
