// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what surface.cpp uses from images_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by surface.cpp only. */
#ifndef LRR_GODS98_SURFACE_FROM_IMAGES_UNIT_HPP
#define LRR_GODS98_SURFACE_FROM_IMAGES_UNIT_HPP

#include "surface_for_images_unit.hpp"

extern RendererRecordTableState s_renderer_record_table;

#endif /* LRR_GODS98_SURFACE_FROM_IMAGES_UNIT_HPP */
