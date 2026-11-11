// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lws uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_LWS_HPP
#define LRR_GODS98_VIEW_FOR_LWS_HPP

namespace unported_ledger_stubs {
// ComInterfaceQueryAndContextDispatch
#if defined(_MSC_VER)
#define LEDGER_SCENE_COM_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define LEDGER_SCENE_COM_STDCALL __attribute__((stdcall))
#else
#define LEDGER_SCENE_COM_STDCALL
#endif
} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {
int ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset( void* animation_owner, int frame_offset);
} // namespace unported_ledger_stubs
extern SharedObjectSoundEnabledFn s_shared_object_sound_enabled;
extern SharedObjectSoundSubmitFn s_shared_object_sound_submit;
extern SceneEntryExpandOps s_scene_entry_expand_ops;
extern SharedObjectCloneOps s_shared_object_clone_ops;

#endif /* LRR_GODS98_VIEW_FOR_LWS_HPP */
