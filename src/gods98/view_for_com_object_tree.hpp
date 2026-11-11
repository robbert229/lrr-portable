// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what com_object_tree uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_COM_OBJECT_TREE_HPP
#define LRR_GODS98_VIEW_FOR_COM_OBJECT_TREE_HPP

#if defined(_MSC_VER)
#define VIEW_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define VIEW_COM_CALL __attribute__((stdcall))
#else
#define VIEW_COM_CALL
#endif

namespace unported_ledger_stubs {
void ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate( void* shared_object, float time);
} // namespace unported_ledger_stubs
extern SceneOwnerLinkReleaseFn s_owner_link_interface_release;
extern SceneOwnerLinkReleaseFn s_owner_link_child_release;
extern SceneActFrameRecordCloneHooks s_scene_act_frame_record_clone_hooks;
extern void* s_scene_material_manager;
void ddiscene_payload_interface_release(void* interface_object);
extern ObjectHierarchyResourceHooks s_object_hierarchy_resource_hooks;

#endif /* LRR_GODS98_VIEW_FOR_COM_OBJECT_TREE_HPP */
