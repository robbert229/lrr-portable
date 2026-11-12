// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what crt.cpp uses from panel_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by crt.cpp only. */
#ifndef LRR_CRT_CRT_FROM_PANEL_UNIT_HPP
#define LRR_CRT_CRT_FROM_PANEL_UNIT_HPP

/* GAME-032 is converted (finding 554): Panel.c's state is panelGlobs in
 * game/panel_unit.h. */
#include "game/panel_unit.h"
#include "gods98/image_adapters.h"
#define s_value_0050080c ((uint32_t)panelGlobs.airMeterOxygenLow)

#endif /* LRR_CRT_CRT_FROM_PANEL_UNIT_HPP */
