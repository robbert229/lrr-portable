// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-025 Loader.c (finding 633): the loading screen and its progress bar.
 *
 * C-compatible header (ADR 0005). loader.c is compiled as C in the matching
 * build; natively it compiles to nothing (no DirectDraw back buffer) and the
 * port's entries serve lego_unit.c (lego_unit_imports.h). Shared C++
 * headers do not include this file: every include and declaration a header
 * adds shifts MSVC's symbol numbering (finding 506).
 * src/game/test_c_boundaries.cpp includes it. */
#ifndef LEGORRR_GAME_LOADER_H
#define LEGORRR_GAME_LOADER_H

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* Load the screens and the bar and read the section-size profile. */
void loader_initialise(const char* loadScreenName, const char* shutdownScreenName,
                       struct TextureSurface* font5HI, const char* profileName,
                       int progressDirection, const char* progressBarName,
                       Area2F* progressWindow, const char* loadingText); /* 0044db50 */
/* Write the measured section sizes to the profile and free the screens. */
void loader_shutdown(const char* profileName);                          /* 0044dd40 */
/* Start the named section's bar (NULL stops drawing). */
void loader_display_loading_bar(const char* section);                   /* 0044dde0 */
/* The file layer's load callback: count the file and redraw the bar. */
void loader_file_load_callback(const char* filename, unsigned int fileSize,
                               void* data);                             /* 0044dec0 */
/* Show the shutdown screen. */
void loader_display_shutdown(void);                                     /* 0044e0a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LOADER_H */
