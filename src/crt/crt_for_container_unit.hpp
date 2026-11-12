// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what container_unit uses from crt.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_CRT_CRT_FOR_CONTAINER_UNIT_HPP
#define LRR_CRT_CRT_FOR_CONTAINER_UNIT_HPP

extern char* s_shared_texture_dir;
extern uint32_t s_renderer_flag_word;
extern void* s_scene_end_hook;
extern void* s_scene_end_value;
extern void* s_scene_sample_hook;
extern void* s_scene_sample_value;

#endif /* LRR_CRT_CRT_FOR_CONTAINER_UNIT_HPP */
