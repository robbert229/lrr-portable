// SPDX-License-Identifier: LGPL-3.0-or-later
/* sound3d.c (original translation unit G98-009): DirectSound sample bank,
 * 3D listener and buffers, and the two streamed wave buffers.
 *
 * C-compatible header (ADR 0005). sound3d.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every declaration a header adds shifts MSVC's symbol numbering
 * (finding 538). The C++ declarations of the same functions are in sound.hpp,
 * audio.hpp, sound_shared.hpp, sound_for_sfx.hpp, simtick.hpp, view.hpp,
 * crt.hpp and resource_from_sound3d.hpp, and src/game/test_c_boundaries.cpp
 * checks them against this header.
 *
 * Only the functions that code outside the unit calls are declared here.
 * They keep the port's contracts, which its callers use; on Win32 the
 * integer frame handles and float bit patterns are the original's
 * pointers and floats. The unit's other functions take the original's
 * arguments in the DirectSound builds (sound3d.c). The functions keep the
 * port's names; the comments give descriptive Sound3D_* names. */
#ifndef LEGORRR_GODS98_SOUND3D_H
#define LEGORRR_GODS98_SOUND3D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EngineContext;

/* Sound3D_ShutDown (FUN_0047a800): restore the primary buffer's volume and
 * release DirectSound. */
void audio_active_shutdown(void);

/* Sound3D_CheckVolumeLimits (FUN_0047a850): 1 for -10000..0 millibels. */
int sound_volume_value_valid(int millibels);

/* Sound3D_Load (FUN_0047a870): load "<name>.wav" into a free sample slot,
 * streamed or into buffers (three if looping); returns the slot or -1. */
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume);

/* Sound3D_Remove (FUN_0047ab00): free a sample slot and its buffers. */
int sound_bank_entry_release(int slot);

/* Sound3D_Update listener tick (FUN_0047aba0): move the listener to the
 * listener frame. The context is the port's ticker argument; the original
 * takes none. */
void simtick_sound_keepalive(struct EngineContext* c);

/* Sound3D_Play2 (FUN_0047ad70): play a sample attached to a frame (mode
 * 0), at a world position (mode 1) or without 3D (mode 2); returns the
 * sound handle (slot * 3 + buffer) or -1. */
int s_play_seam(int mode, int position, int slot, int loop,
                const float* xyz);

/* Sound3D_SetVolume / Sound3D_GetVolume (FUN_0047b050, FUN_0047b0d0). */
void sound_bank_entry_volume_set(int slot, int millibels);
int sound_bank_entry_volume(int slot);

/* Sound3D_StopSound (FUN_0047b130): stop one buffer by handle. */
void sound_buffer_stop_handle(int buffer_handle);

/* Sound3D_StopAllSounds (FUN_0047b160). */
void sound_subsystem_stop_all(void);

/* Sound3D_RecurseRemoveSound (FUN_0047b330): unlink and free the frame
 * sound record of a buffer; returns whether the head matched. */
int nested_record_list_remove_matching_key_impl(void* owner, void* key,
                                                void* opaque_record);

/* Sound3D_Update (FUN_0047b390): move the attached sounds, drop finished
 * ones and refill the two streams. */
void sound_source_spatial_refresh_all(void);

/* Sound3D_SetMinDistForAtten (FUN_0047b4a0). */
void sound_tracked_buffers_set_min_distance(float value);

/* Sound3D_SetGlobalVolume (FUN_0047b500) and its slider-index form
 * (FUN_0047b530). */
void audio_sound_volume_set(int millibels);
void audio_sound_volume_index_set(int index);

/* Sound3D_GetSamplePlayTime (FUN_0047b8e0): the sample's length in
 * seconds; 0 for slot -1. */
float sound_streamed_wave_buffer_step_rate(int slot);

/* Sound3D_Stream_Stop (FUN_0047ba20): stream 0 or the looping stream 1. */
int sound_stream_stop_and_release(int stream_index);

/* Sound3D_CheckVectorsEqual (FUN_0047c0c0). */
int vec_exact_equal(const float* a, const float* b);

/* Sound3D_MinVolume (FUN_0047c160): DSBVOLUME_MIN. */
int32_t constant_negative_10000_get(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SOUND3D_H */
