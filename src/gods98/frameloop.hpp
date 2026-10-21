// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — native frame loop (the visible-milestone driver).
 *
 * Mirrors the corpus's WinMain frame loop (finding 02/20) at the level the
 * reconstruction can stand today:
 *
 *   mount wads (exe-basename prefix) -> window + DDraw surface (platform
 *   seam + miniwin) -> load a couple of real corpus BMPs through the texture
 *   loader -> loop { pump events; EngineTick; lock; draw; present } ->
 *   EngineShutdown + teardown.
 *
 * Env knobs (headless/CI verification):
 *   LEGORRR_WAD_DIR        directory containing LegoRR*.wad (default: cwd)
 *   LEGORRR_AUTOSTOP_FRAMES  stop after N frames (0 = until quit/ESC)
 *   LEGORRR_SCREENSHOT      write a PPM of the last frame to this path
 *
 * The presentation step (miniwin has no surface Flip implementation) is the
 * platform glue's SDL_UpdateWindowSurface — documented divergence; the winapi
 * reference build keeps real DDraw Flip instead.
 */
#ifndef LEGORRR_FRAMELOOP_HPP
#define LEGORRR_FRAMELOOP_HPP

/* Run the engine frame loop on a window. Returns 0 on clean exit. */
int LegorrrRun(void);

#endif /* LEGORRR_FRAMELOOP_HPP */