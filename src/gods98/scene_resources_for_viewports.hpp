// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what viewports uses from scene_resources.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SCENE_RESOURCES_FOR_VIEWPORTS_HPP
#define LRR_GODS98_SCENE_RESOURCES_FOR_VIEWPORTS_HPP

#if defined(_MSC_VER)
#define SCENE_RESOURCE_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_RESOURCE_COM_CALL __attribute__((stdcall))
#else
#define SCENE_RESOURCE_COM_CALL
#endif

namespace lego {
extern int s_slots_live;
extern void** s_interface_forward_sync_items;
extern uint32_t s_interface_forward_sync_items_capacity;
} // namespace lego

#endif /* LRR_GODS98_SCENE_RESOURCES_FOR_VIEWPORTS_HPP */
