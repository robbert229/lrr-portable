// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what sound3d uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_SOUND3D_HPP
#define LRR_GODS98_SIMTICK_FOR_SOUND3D_HPP

extern int s_sound_on;
extern int s_sound_active;
extern int (*s_keepalive)(int, int, int);

#endif /* LRR_GODS98_SIMTICK_FOR_SOUND3D_HPP */
