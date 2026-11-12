// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mesh_unit uses from record.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RECORD_FOR_MESH_UNIT_HPP
#define LRR_GODS98_RECORD_FOR_MESH_UNIT_HPP

#if defined(_MSC_VER)
#define RECORD_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define RECORD_COM_CALL __attribute__((stdcall))
#else
#define RECORD_COM_CALL
#endif

#endif /* LRR_GODS98_RECORD_FOR_MESH_UNIT_HPP */
