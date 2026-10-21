// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — software level renderer (finding 22 §3).
 *
 * D3DRM is confirmed software-only, so the reconstruction can rasterize the
 * level itself. `level_render_map` draws the terrain grid (from the voxel
 * array the .map pipeline built) and the placed entities (from the entity
 * tables) into a locked RGBA32 (byte order R,G,B,A) framebuffer. Colours
 * are the port's own palette — derived from the documented cell semantics
 * (finding 07/13) rather than a byte clone of the shipped palette.
 *
 * Cell → pixel is orthographic: cell (cx,cy) maps to pixel
 * (cx/W*w, cy/H*h) with a level-relative->pixel scale, matching how the
 * selection demo maps the mouse back to cells (select.cpp). Entities draw as
 * type-coloured markers with a yellow ring when selected; the tracked mouse
 * cell gets a white outline so picking is visible.
 */
#ifndef LEGORRR_GAME_RENDER_HPP
#define LEGORRR_GAME_RENDER_HPP

#include "game.hpp"
#include "level.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Draw the whole frame: terrain grid + entity markers + cursor cell.
 * `pixels`/`pitch` are the locked framebuffer (RGBA32 R,G,B,A); `w`/`h` is
 * its size in pixels. `mouse_x`/`mouse_y` (>= 0) outline the cursor cell. */
void level_render_map(const LevelState* lvl, const GameContext* ctx,
                      int mouse_x, int mouse_y,
                      int w, int h, void* pixels, int pitch);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_RENDER_HPP */