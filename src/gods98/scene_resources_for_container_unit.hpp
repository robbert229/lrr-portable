// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what container_unit uses from scene_resources.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SCENE_RESOURCES_FOR_CONTAINER_UNIT_HPP
#define LRR_GODS98_SCENE_RESOURCES_FOR_CONTAINER_UNIT_HPP

namespace lego {
extern InterfaceArrayForwardSyncHooks s_interface_array_forward_sync_hooks;
} // namespace lego

#endif /* LRR_GODS98_SCENE_RESOURCES_FOR_CONTAINER_UNIT_HPP */
