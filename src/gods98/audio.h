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

#include <miniwin/d3d.h>
#include <miniwin/windows.h>

struct IDirect3DRMFrame3;

#ifdef __cplusplus
extern "C" {
#endif

/* Sound3D_Initialise (FUN_0047a640): start DirectSound on window. */
int audio_system_init(void* window);

#define SOUND3D_MAXUPDATEFRAMES 20

/* The frame state after the initialised flag (0053387c, 0x88 bytes on
 * Win32). */
typedef struct Sound3DFrameGlobs {
    LONG windowsVolume;                          /* +0x00 */
    unsigned long reserved_04;
    struct IDirect3DRMFrame3* updateFrameList[SOUND3D_MAXUPDATEFRAMES]; /* +0x08 */
    unsigned long reserved_58;
    D3DVECTOR oldPos;                        /* +0x5c */
    unsigned long reserved_68;
    D3DVECTOR oldOrientFront;                /* +0x6c */
    unsigned long reserved_78;
    D3DVECTOR oldOrientUp;                   /* +0x7c */
} Sound3DFrameGlobs;

/* DAT_00533878: set while 3D sound is running. It is the first word of
 * Sound3D.c's state after s_sound3d, followed by the frame state. */
typedef struct Sound3D_State {
    int initialised;                 /* 00533878 */
    Sound3DFrameGlobs frames;        /* 0053387c */
} Sound3D_State;
extern Sound3D_State sound3dState;
#define s_bank_sound_on (sound3dState.initialised)

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_AUDIO_H */
