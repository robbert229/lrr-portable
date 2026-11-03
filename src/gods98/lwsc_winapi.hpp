// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_LWSC_WINAPI_HPP
#define LEGORRR_LWSC_WINAPI_HPP

#include "lwsc_scene.hpp"

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64) && \
    (defined(_M_IX86) || defined(__i386__))
#define LWSC_WINAPI_POSE_BACKEND 1

typedef struct LwscWinapiContext LwscWinapiContext;

/* Main scene-owner thread only; no concurrent pool/hook/root changes.
 * root must be the actual initialized scene_root_object_get() pooled object.
 * This does not initialize RM, install hooks or select/load another runtime.
 * The context retains all three actual application-root interfaces and pins
 * their already-loaded D3DRM module. Each new authored frame is channel0 of
 * its OWN genuine pooled DDi object; no lookup-time replacement frames.
 * The complete producer template/Model associations remain instance-owned.
 *
 * REQUIRED order: retire borrowed DDi consumers/caches -> destroy instances
 * -> destroy contexts -> scene_runtime_shutdown/pool shutdown. The runtime
 * owner MUST honor lwsc_winapi_can_shutdown before global shutdown. The
 * existing global shutdown path has no guard installation API; its separate
 * proposed bridge is required for enforcement there. Root loss poisons this
 * context's operations/lookups; retained interfaces permit cleanup but cannot
 * make already-exported pool-record pointers valid after a pool shutdown. */
LwscWinapiContext* lwsc_winapi_context_create(void* application_ddi_root,
                                            LwscError* error);

/* Supplies only real frame/name/matrix/world-query operations. All renderer,
 * opacity and activity visibility operations stay NULL. Use explicitly with
 * lwsc_instance_create_for_pose; full rendering creation stays strict.
 * parent is borrowed and valid through successful context destruction.
 * On failure outputs are cleared. Context must outlive ops/parent/instances. */
int lwsc_winapi_backend_get(LwscWinapiContext* context, LwscBackendOps* ops,
                            const LwscFrameRef** parent);

/* Returns the SAME pool-owned object whose channel0 is this exact live
 * registered producer ref. Does not allocate, add a new frame, reparent or
 * fabricate a wrapper. Foreign/copied/cleared/destroyed refs return NULL;
 * the application's parent ref is not a selected-node fallback.
 * BORROWED: caller must not free/destroy the DDi object, change its property,
 * kind/channels, or reparent the frame. Valid only while its instance/frame
 * and context/root/pool remain alive and no mutation failure has occurred.
 * Clear cached consumers before instance teardown. Direct DDi consumers must
 * stop on producer/backend failure; a borrowed void* cannot revoke itself. */
void* lwsc_winapi_scene_object(LwscWinapiContext* context,
                               const LwscFrameRef* frame);

/* Refuses busy destruction: returns0 without freeing context if registered
 * frames (including any deferred parent releases) remain. NULL is harmless.
 * On success releases retained root interfaces, module pin and context.
 * The caller must then clear its own context/ops/parent pointers. */
int lwsc_winapi_context_destroy(LwscWinapiContext* context);
unsigned int lwsc_winapi_frame_count(const LwscWinapiContext* context);
const char* lwsc_winapi_context_error(const LwscWinapiContext* context);

/* A shutdown barrier for the real scene owner, not an installer/hook.
 * Returns1 only after ALL contexts have been destroyed, even empty ones.
 * Direct pool shutdown while false violates the ownership contract. */
int lwsc_winapi_can_shutdown(void);

#endif /* x86 WinAPI */
#endif /* LEGORRR_LWSC_WINAPI_HPP */
