// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 scene-object records.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * record functions that units rebuilt as C call. The C++ code keeps using
 * gods98/record.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_RECORD_H
#define LEGORRR_GODS98_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

/* SceneRenderLockToggle (FUN_00473b40): with enabled, switch the object to
 * its locked render pair and set flag bit 3; without, switch back. */
void scene_render_lock_toggle(void* scene_object, int enabled);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_RECORD_H */
