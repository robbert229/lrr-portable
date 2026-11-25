// SPDX-License-Identifier: LGPL-3.0-or-later
/* The frame rate the FPS overlay shows ([divergence:fps-overlay], tweaks:
 * fps_overlay in platform/native/config.h): frames counted over windows of
 * half a second, so the number is steady enough to read. Port-only, below
 * the seam; the overlay's drawing is in gods98/main.c. */
#ifndef LEGORRR_PLATFORM_NATIVE_FPS_COUNTER_H
#define LEGORRR_PLATFORM_NATIVE_FPS_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* The length of a counting window, in milliseconds. */
#define LRR_FPS_WINDOW_MS 500u

/* Counts one frame, finished at nowMs (SDL_GetTicks's clock), and returns
 * the frame rate of the last full window: 0 until the first window is
 * done. */
float LRR_FpsFrame(unsigned long nowMs);

/* Forgets the count, as when the clock jumps or the overlay turns on. */
void LRR_FpsReset(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_PLATFORM_NATIVE_FPS_COUNTER_H */
