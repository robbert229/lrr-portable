// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mesh_unit uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_MESH_UNIT_HPP
#define LRR_GODS98_VIEW_FOR_MESH_UNIT_HPP

extern EngineTaskCallbackHooks s_engine_task_callback_hooks;

#endif /* LRR_GODS98_VIEW_FOR_MESH_UNIT_HPP */
