// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from crt.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_CRT_CRT_FOR_LEGO_HPP
#define LRR_CRT_CRT_FOR_LEGO_HPP

extern float s_global_float_00556f00;

#endif /* LRR_CRT_CRT_FOR_LEGO_HPP */
