// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what sound3d uses from audio.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_AUDIO_FOR_SOUND3D_HPP
#define LRR_GODS98_AUDIO_FOR_SOUND3D_HPP

extern AudioPlayer* s_active_audio_player;
extern float s_listener_rolloff;
extern AudioSystemInitHook s_audio_system_init_hook;
extern void* s_audio_system_init_context;
int audio_stream_submit_chunk(AudioPlayer* p, int stream_index);

#endif /* LRR_GODS98_AUDIO_FOR_SOUND3D_HPP */
