// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 3D sound (original translation unit G98-009, finding 505; the
 * port's audio.cpp and sound.cpp): the functions and globals that units
 * rebuilt as C use.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of what the
 * C++ headers gods98/audio.hpp and gods98/sound.hpp declare with extern "C".
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_AUDIO_H
#define LEGORRR_GODS98_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Sound3D_Initialise (FUN_0047a640): start DirectSound on window. */
int audio_system_init(void* window);

/* DAT_00533878: set while 3D sound is running. */
extern int s_bank_sound_on;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_AUDIO_H */
