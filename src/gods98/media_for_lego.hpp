// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from media.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_MEDIA_FOR_LEGO_HPP
#define LRR_GODS98_MEDIA_FOR_LEGO_HPP

extern int s_cd_base;
extern int s_cd_window;
extern unsigned s_cd_slot;
extern void* s_mind_context;
extern int s_mind_mode;
extern float s_mind_listener;
extern float s_mind_clip;
extern float s_mind_ref;
extern float s_mind_cell_scale;
extern float s_mind_clip_base;
extern float s_mind_ref_base;
extern float s_mind_dist_base;

#endif /* LRR_GODS98_MEDIA_FOR_LEGO_HPP */
