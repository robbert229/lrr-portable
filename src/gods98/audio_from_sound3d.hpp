// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what audio.cpp uses from sound3d.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by audio.cpp only. */
#ifndef LRR_GODS98_AUDIO_FROM_SOUND3D_HPP
#define LRR_GODS98_AUDIO_FROM_SOUND3D_HPP

extern "C" int s_sfx_attenuation[11];

#endif /* LRR_GODS98_AUDIO_FROM_SOUND3D_HPP */
