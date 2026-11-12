// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_LWSC_MINIWIN_HPP
#define LEGORRR_LWSC_MINIWIN_HPP

#include "lwsc_scene.hpp"

#if defined(LEGORRR_BACKEND_MINIWIN)

/* LWSC frame backend on miniwin's D3DRM frames (native builds).
 *
 * Every authored frame is a real IDirect3DRMFrame2 created under its parent
 * with IDirect3DRM::CreateFrame; LwscFrameRef::interface_pointer is that
 * frame. miniwin frames only support replacing their transform and reading
 * their local translation, so the backend keeps each frame's local matrix
 * (in the frame's AppData record) and composes ancestors itself. Matrices
 * use the D3DRM row-vector layout: translation in [12..14].
 *
 * Mesh attachments keep the authored Model with its opacity; activity
 * visibility hides a whole subtree. lwsc_miniwin_meshes_visit walks the
 * visible meshes for the software scene renderer.
 *
 * One process-wide context; main thread only. */
int lwsc_miniwin_backend_get(LwscBackendOps* ops, const LwscFrameRef** parent);

/* Nonzero when `frame` is a live frame created by this backend. */
int lwsc_miniwin_frame_known(const void* frame);

/* The transform from `frame` space to `ancestor` space: the product of the
 * local matrices from `frame` up to, but excluding, `ancestor`. A NULL
 * ancestor means the backend's scene root. Returns 0 when `ancestor` is
 * not an ancestor of `frame`. */
int lwsc_miniwin_frame_transform_get(const void* frame, const void* ancestor,
                                     float matrix[16]);

typedef void (*LwscMiniwinMeshVisitFn)(void* context, const Model* model,
                                       const float model_to_ancestor[16],
                                       float opacity);
/* Visit every mesh in the visible subtree under `top` (inclusive), with
 * matrices relative to `ancestor` (see above). Returns the mesh count. */
unsigned int lwsc_miniwin_meshes_visit(const void* top, const void* ancestor,
                                       LwscMiniwinMeshVisitFn visit,
                                       void* context);

#endif /* LEGORRR_BACKEND_MINIWIN */
#endif /* LEGORRR_LWSC_MINIWIN_HPP */
