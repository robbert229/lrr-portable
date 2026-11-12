// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from frameloop.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_FRAMELOOP_FOR_FRONTEND_UNIT_HPP
#define LRR_GODS98_FRAMELOOP_FOR_FRONTEND_UNIT_HPP

int s_icontains(const char* haystack, const char* needle);
void frameloop_blit_texture_scaled(const TextureSurface* texture, void* pixels, int pitch, int width, int height, int x, int y, int draw_w, int draw_h);

#endif /* LRR_GODS98_FRAMELOOP_FOR_FRONTEND_UNIT_HPP */
