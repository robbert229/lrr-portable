// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Init.c (original translation unit LATE-002, finding 613): the
 * start-up display selection. Init_Initialise enumerates the DirectDraw
 * drivers, then either picks the best hardware device and mode by itself
 * or runs the "Mode Selection" dialog (resource 101), and sets up the
 * display through DirectDraw.c.
 *
 * C-compatible boundary header (ADR 0005). init.c is compiled as C in the
 * matching build and as C++ in the MinGW reference build. The native port
 * has no DirectDraw: there the unit defines only its state. Shared C++ headers
 * do not include this file: every include and declaration a header adds
 * shifts MSVC's symbol numbering (finding 506).
 * src/game/test_c_boundaries.cpp includes it. */
#ifndef LEGORRR_GODS98_INIT_H
#define LEGORRR_GODS98_INIT_H

#include "directdraw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INIT_MAX_DRIVERS 20
#define INIT_MAX_DEVICES 20
#define INIT_MAX_MODES 200
#define INIT_MAX_VALID_MODES 200

/* The unit's state: one common symbol at 005580a0 (0x1d444 bytes). The
 * three lists are the ones DirectDraw.c's enumerations fill; validModes
 * holds the width, height and depth filters that Init_AddValidMode
 * registers (a zero field matches anything). */
typedef struct Init_Globs {
    unsigned int driverCount;                       /* +0x00000 005580a0 */
    unsigned int deviceCount;                       /* +0x00004 005580a4 */
    unsigned int modeCount;                         /* +0x00008 005580a8 */
    DirectDraw_Driver drivers[INIT_MAX_DRIVERS];    /* +0x0000c 005580ac */
    DirectDraw_Device devices[INIT_MAX_DEVICES];    /* +0x0159c 0055963c */
    DirectDraw_Mode modes[INIT_MAX_MODES];          /* +0x02b2c 0055abcc */
    DirectDraw_Driver* selDriver;                   /* +0x0ffac 0056804c */
    DirectDraw_Device* selDevice;                   /* +0x0ffb0 00568050 */
    DirectDraw_Mode* selMode;                       /* +0x0ffb4 00568054 */
    int selFullScreen;                              /* +0x0ffb8 00568058 */
    int wasFullScreen;                              /* +0x0ffbc 0056805c */
    DirectDraw_Mode validModes[INIT_MAX_VALID_MODES]; /* +0x0ffc0 00568060 */
    unsigned int validModeCount;                    /* +0x1d440 005754e0 */
} Init_Globs;

/* Defined in every build: the native port keeps its valid-mode filters
 * in it too (init_port.cpp). */
extern Init_Globs initGlobs;

#if !defined(LEGORRR_BACKEND_MINIWIN)
/* 0049d030: set up the display. setup runs the dialog, debug also accepts
 * 800x600 and 1024x768, best picks the last driver with a hardware device
 * and a valid mode without asking, window selects a window in that case.
 * noHALMsg is shown when best finds no hardware device. Returns
 * DirectDraw_Setup's result, or 0. */
int init_initialise(int setup, int debug, int best, int window,
                    const char* noHALMsg);                            /* 0049d030 */
/* 0049d2f0 (the dialog procedure) and the dialog helpers 0049d5e0,
 * 0049d630, 0049d780 and 0049d9d0 take an HWND; init.c declares them. */
void init_add_valid_mode(unsigned int width, unsigned int height,
                         unsigned int depth);                         /* 0049d870 */
int init_is_valid_mode(unsigned int mode);                            /* 0049d8d0 */
int init_get_mode(const char* name, unsigned int* mode);              /* 0049d950 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_INIT_H */
