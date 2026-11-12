// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what avi_player uses from crt.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_CRT_CRT_FOR_AVI_PLAYER_HPP
#define LRR_CRT_CRT_FOR_AVI_PLAYER_HPP

/* FUN_0047e7e0: the corpus's second identical position-loop body. */
#if defined(_MSC_VER)
#define LEGORRR_CRT_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define LEGORRR_CRT_STDCALL __attribute__((stdcall))
#else
#define LEGORRR_CRT_STDCALL
#endif

#endif /* LRR_CRT_CRT_FOR_AVI_PLAYER_HPP */
