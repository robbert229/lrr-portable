// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * SFX.c and Files.c entries declared in sound.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_SOUND_API_H
#define LEGORRR_GODS98_SOUND_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int sound_sample_file_open_by_name(const char* name);      /* 0047fb60 */
void sound_handle_stop_forward(int buffer_handle);         /* 00465190 */
void sound_subsystem_frame_tick(float dt);                 /* 004651a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SOUND_API_H */
