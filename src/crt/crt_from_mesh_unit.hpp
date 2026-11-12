// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what crt.cpp uses from mesh_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by crt.cpp only. */
#ifndef LRR_CRT_CRT_FROM_MESH_UNIT_HPP
#define LRR_CRT_CRT_FROM_MESH_UNIT_HPP

extern void* s_model_render_list_head;

#endif /* LRR_CRT_CRT_FROM_MESH_UNIT_HPP */
