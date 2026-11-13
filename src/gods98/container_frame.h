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
 * last one 0). Natively sound3d.c uses the port's; in the matching build it
 * calls the unit with eleven. */
#ifndef LEGORRR_GODS98_CONTAINER_FRAME_H
#define LEGORRR_GODS98_CONTAINER_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: container.c's Container_Frame_SetAppData serves this
 * name (container_link_names.h, finding 624), with the original's eleven
 * arguments; the macro passes the last one, 0, as the original does. A
 * macro and an unnamed parameter leave sound3d.c's symbol numbering as it
 * was (finding 538). */
void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20,
                                    const uint32_t*);
#define ddiscene_animation_hook_create(a, b, c, d, e, f, g, h, i, j) \
    ddiscene_animation_hook_create(a, b, c, d, e, f, g, h, i, j, 0)
#else
void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_FRAME_H */
