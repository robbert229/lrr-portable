// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what viewports uses from surface.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SURFACE_FOR_VIEWPORTS_HPP
#define LRR_GODS98_SURFACE_FOR_VIEWPORTS_HPP

extern void* s_screen_bounded_surface_device;
int surface_screen_dimensions_get(uint32_t* width_out, uint32_t* height_out);

#endif /* LRR_GODS98_SURFACE_FOR_VIEWPORTS_HPP */
