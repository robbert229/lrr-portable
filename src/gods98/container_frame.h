// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for Container_Frame_SetAppData (FUN_00475f70;
 * the port's ddiscene_animation_hook_create in gods98/view.cpp), which
 * gods98/sound3d.c calls. It is not added to gods98/view.h or
 * gods98/container.h because other C units include them, and every
 * declaration shifts their symbol numbering (finding 538). Shared C++
 * headers do not include this file; src/game/test_c_boundaries.cpp checks
 * it against view.hpp.
 *
 * The port's contract has ten arguments; the original pushes eleven (the
 * last one 0). sound3d.c uses the port's. */
#ifndef LEGORRR_GODS98_CONTAINER_FRAME_H
#define LEGORRR_GODS98_CONTAINER_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_FRAME_H */
