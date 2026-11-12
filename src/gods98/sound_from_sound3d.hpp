// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what sound.cpp uses from sound3d.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by sound.cpp only. */
#ifndef LRR_GODS98_SOUND_FROM_SOUND3D_HPP
#define LRR_GODS98_SOUND_FROM_SOUND3D_HPP

extern void* s_buffer_assoc_head;

#endif /* LRR_GODS98_SOUND_FROM_SOUND3D_HPP */
