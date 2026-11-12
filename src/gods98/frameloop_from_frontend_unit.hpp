// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frameloop.cpp uses from frontend_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by frameloop.cpp only. */
#ifndef LRR_GODS98_FRAMELOOP_FROM_FRONTEND_UNIT_HPP
#define LRR_GODS98_FRAMELOOP_FROM_FRONTEND_UNIT_HPP

void menu_save_image_config_rect_blit(FrontEnd* fe, GameContext* game, int item_index, void* pixels, int pitch, int width, int height);
void menu_overlay_surface_and_text_setup( FrontEnd* fe, const TextureSurface* hi_font, const TextureSurface* lo_font, void* pixels, int pitch, int width, int height, int draw_cursor);

#endif /* LRR_GODS98_FRAMELOOP_FROM_FRONTEND_UNIT_HPP */
