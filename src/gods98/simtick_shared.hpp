// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/simtick.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_SIMTICK_SHARED_HPP
#define LRR_GODS98_SIMTICK_SHARED_HPP

extern SimtickEffect s_effects[16];
extern int s_effect_count;
extern int s_effect_fired;
extern void (*s_effect_opacity_hook)(uint32_t, int, float, int);
extern void (*s_effect_release_hook)(uint32_t, int);

#endif /* LRR_GODS98_SIMTICK_SHARED_HPP */
