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

/* DAT_00533878: set while 3D sound is running. In the matching build it is
 * the first word of Sound3D.c's state after s_sound3d, followed by the
 * frame state (0x88 bytes, typed in sound3d.c): one struct, which is how
 * the original's frame state sits 4-aligned at 0053387c. */
typedef struct Sound3D_State {
    int initialised;                 /* 00533878 */
    unsigned char frames[0x88];      /* 0053387c */
} Sound3D_State;
extern Sound3D_State sound3dState;
#define s_bank_sound_on (sound3dState.initialised)

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_AUDIO_H */
