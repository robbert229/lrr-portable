// SPDX-License-Identifier: LGPL-3.0-or-later
/* The audio library of ADR 0007's substitution table: SDL3 audio in place
 * of DirectSound (sound3d.c, G98-009), audio files decoded by miniaudio in
 * place of WinMM's mmio and MCI "cdaudio" device (soundsys.c, G98-021), and
 * a mixer of one CD-audio line in place of WinMM's mixer (main.c, G98-005).
 * The units call it at their `// [library:audio]` call sites; it holds no
 * game logic. Each function stands for the one Win32 call named beside it,
 * with that call's arguments and results, so a call site changes only the
 * name it calls.
 *
 * DirectSound. A buffer is its PCM bytes and an SDL3 audio stream that the
 * stream's get callback feeds from them, from the buffer's play cursor and
 * wrapping when the buffer loops. SDL converts each stream's format to the
 * device's and mixes the streams bound to the device, so the library needs
 * no mixer of its own. The primary buffer's volume is the device's gain.
 * A 3D buffer is attenuated by its distance from the listener, as
 * DirectSound's default rolloff does, and panned by the sine of its
 * azimuth about the listener's orientation, with DirectSound's pan law:
 * the far channel is attenuated and the near one keeps its level, so a
 * centred source sounds as a 2D one does. The far channel's gain is
 * 1 - |sin azimuth|, hard over at 90 degrees. Microsoft documents no
 * curve for its software 3D pan; this one approximates it. There is no
 * Doppler shift: it follows the buffers' and listener's velocities, which
 * the units never set, so the original never shifts either. Feeding a
 * stream converts the buffer's 8- or 16-bit mono or stereo PCM to float
 * stereo with the channels' gains, ramped across each feed.
 *
 * mmio reads a whole file (or the caller's memory) and walks its RIFF
 * chunks in place.
 *
 * MCI cdaudio plays the disc's audio tracks from files: track N is
 * `Music/TrackNN` (two digits) with the extension .ogg, .flac, .mp3 or
 * .wav, in the working directory, matched without case. With no such
 * files there is no disc: "open cdaudio" fails, as the original's does
 * without a CD, and the game plays no music.
 *
 * The mixer has one device, whose speakers destination has one source:
 * the CD-audio line, two channels with one volume fader from 0 to 65535.
 * The fader scales the cdaudio device's tracks by value / 65535 per
 * channel; it starts at its maximum. The DirectSound primary buffer's
 * volume does not reach it, as on Windows. */
#ifndef LEGORRR_LIBRARY_AUDIO_SDL_H
#define LEGORRR_LIBRARY_AUDIO_SDL_H

#include "gods98/winmm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- DirectSound ---------------------------------------------------------- */

typedef struct AudioDevice AudioDevice;     /* IDirectSound */
typedef struct AudioBuffer AudioBuffer;     /* IDirectSoundBuffer and its
                                             * IDirectSound3DBuffer */
typedef struct AudioListener AudioListener; /* IDirectSound3DListener */

/* The HRESULTs the library returns. */
#define AUDIO_OK 0
#define AUDIO_ERR_NODRIVER ((long)0x88780078)
#define AUDIO_ERR_BADFORMAT ((long)0x88780064)
#define AUDIO_ERR_INVALIDPARAM ((long)0x80070057)
#define AUDIO_ERR_OUTOFMEMORY ((long)0x8007000e)

/* DSBCAPS_PRIMARYBUFFER, DSBPLAY_LOOPING and the GetStatus bits. */
#define AUDIO_BUFFER_PRIMARY 0x1
#define AUDIO_PLAY_LOOPING 0x1
#define AUDIO_STATUS_PLAYING 0x1
#define AUDIO_STATUS_LOOPING 0x4
/* DS3DMODE_HEADRELATIVE: positions are relative to the listener.
 * DS3DMODE_DISABLE: a 3D buffer that plays without attenuation or pan. */
#define AUDIO_3D_MODE_HEADRELATIVE 1
#define AUDIO_3D_MODE_DISABLE 2

/* DirectSoundCreate: opens the default playback device. */
long audio_create(AudioDevice** device);
/* IDirectSound::Release: closes the device. */
unsigned long audio_release(AudioDevice* device);
/* IDirectSound::SetCooperativeLevel. */
long audio_set_cooperative_level(AudioDevice* device, void* window,
                                 DWORD level);
/* IDirectSound::CreateSoundBuffer: the primary buffer, or a PCM buffer of
 * bytes in format, silent until written. */
long audio_create_buffer(AudioDevice* device, DWORD flags, DWORD bytes,
                         const WAVEFORMATEX* format, AudioBuffer** buffer);
/* IDirectSound::DuplicateSoundBuffer: a buffer sharing original's data. */
long audio_duplicate_buffer(AudioDevice* device, AudioBuffer* original,
                            AudioBuffer** duplicate);

/* IDirectSoundBuffer::QueryInterface for IID_IDirectSound3DListener (on
 * the primary buffer) and IID_IDirectSound3DBuffer. */
long audio_buffer_query_listener(AudioBuffer* buffer,
                                 AudioListener** listener);
long audio_buffer_query_3d(AudioBuffer* buffer, AudioBuffer** buffer3D);
/* IDirectSoundBuffer::Release and IDirectSound3DBuffer::Release. */
unsigned long audio_buffer_release(AudioBuffer* buffer);
long audio_buffer_set_format(AudioBuffer* buffer, const WAVEFORMATEX* format);
long audio_buffer_play(AudioBuffer* buffer, DWORD reserved1, DWORD priority,
                       DWORD flags);
long audio_buffer_stop(AudioBuffer* buffer);
long audio_buffer_set_current_position(AudioBuffer* buffer, DWORD position);
long audio_buffer_get_current_position(AudioBuffer* buffer, DWORD* play,
                                       DWORD* write);
long audio_buffer_get_status(AudioBuffer* buffer, DWORD* status);
/* Volumes are DirectSound's: hundredths of a decibel, -10000 to 0. */
long audio_buffer_set_volume(AudioBuffer* buffer, LONG volume);
long audio_buffer_get_volume(AudioBuffer* buffer, LONG* volume);
long audio_buffer_get_frequency(AudioBuffer* buffer, DWORD* frequency);
/* Lock and Unlock: Lock holds the buffer's stream until Unlock, so the
 * mix never reads bytes being written. */
long audio_buffer_lock(AudioBuffer* buffer, DWORD offset, DWORD bytes,
                       void** audio1, DWORD* bytes1, void** audio2,
                       DWORD* bytes2, DWORD flags);
long audio_buffer_unlock(AudioBuffer* buffer, void* audio1, DWORD bytes1,
                         void* audio2, DWORD bytes2);
long audio_buffer_restore(AudioBuffer* buffer);

/* IDirectSound3DBuffer. Every setting applies at once; apply is ignored. */
long audio_buffer_set_3d_mode(AudioBuffer* buffer, DWORD mode, DWORD apply);
long audio_buffer_set_3d_position(AudioBuffer* buffer, float x, float y,
                                  float z, DWORD apply);
long audio_buffer_set_min_distance(AudioBuffer* buffer, float distance,
                                   DWORD apply);
long audio_buffer_set_max_distance(AudioBuffer* buffer, float distance,
                                   DWORD apply);

/* IDirectSound3DListener. */
long audio_listener_set_position(AudioListener* listener, float x, float y,
                                 float z, DWORD apply);
long audio_listener_set_orientation(AudioListener* listener, float frontX,
                                    float frontY, float frontZ, float topX,
                                    float topY, float topZ, DWORD apply);
long audio_listener_set_doppler_factor(AudioListener* listener, float factor,
                                       DWORD apply);
long audio_listener_set_rolloff_factor(AudioListener* listener, float factor,
                                       DWORD apply);
long audio_listener_commit_deferred_settings(AudioListener* listener);

/* --- mmio ----------------------------------------------------------------- */

/* mmioOpenA: a host path, or with info->fccIOProc FOURCC_MEM the memory
 * that info describes, which the caller keeps alive until mmioClose. */
HMMIO audio_mmio_open(const char* path, LPMMIOINFO info, DWORD flags);
MMRESULT audio_mmio_close(HMMIO hmmio, UINT flags);
LONG audio_mmio_read(HMMIO hmmio, HPSTR buffer, LONG bytes);
LONG audio_mmio_seek(HMMIO hmmio, LONG offset, int origin);
MMRESULT audio_mmio_get_info(HMMIO hmmio, LPMMIOINFO info, UINT flags);
MMRESULT audio_mmio_set_info(HMMIO hmmio, const MMIOINFO* info, UINT flags);
MMRESULT audio_mmio_advance(HMMIO hmmio, LPMMIOINFO info, UINT flags);
MMRESULT audio_mmio_descend(HMMIO hmmio, LPMMCKINFO chunk,
                            const MMCKINFO* parent, UINT flags);
MMRESULT audio_mmio_ascend(HMMIO hmmio, LPMMCKINFO chunk, UINT flags);

/* --- MCI cdaudio ------------------------------------------------------------ */

/* mciSendStringA for the commands the units send: "open cdaudio",
 * "set cdaudio time format tmsf", "play cdaudio from N", "status cdaudio
 * mode", "status cdaudio current track", "stop cdaudio" and "close
 * cdaudio". */
MCIERROR audio_mci_send_string(LPCSTR command, LPSTR result, UINT capacity,
                               HWND callback);
/* mciGetErrorStringA. */
BOOL audio_mci_get_error_string(MCIERROR error, LPSTR text, UINT capacity);

/* --- mixer ------------------------------------------------------------------ */

/* mixerGetNumDevs, mixerOpen, mixerClose, mixerGetDevCapsA,
 * mixerGetLineInfoA, mixerGetLineControlsA, mixerGetControlDetailsA and
 * mixerSetControlDetails, on the one device described above. */
UINT audio_mixer_get_num_devs(void);
MMRESULT audio_mixer_open(HMIXER* mixer, UINT device, DWORD_PTR callback,
                          DWORD_PTR instance, DWORD flags);
MMRESULT audio_mixer_close(HMIXER mixer);
MMRESULT audio_mixer_get_dev_caps(UINT_PTR device, MIXERCAPSA* caps, UINT size);
MMRESULT audio_mixer_get_line_info(HMIXEROBJ mixer, MIXERLINEA* line,
                                   DWORD flags);
MMRESULT audio_mixer_get_line_controls(HMIXEROBJ mixer,
                                       MIXERLINECONTROLSA* controls,
                                       DWORD flags);
MMRESULT audio_mixer_get_control_details(HMIXEROBJ mixer,
                                         MIXERCONTROLDETAILS* details,
                                         DWORD flags);
MMRESULT audio_mixer_set_control_details(HMIXEROBJ mixer,
                                         MIXERCONTROLDETAILS* details,
                                         DWORD flags);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The library's arithmetic, for its tests. */
namespace audio_sdl_detail {
/* DirectSound's volume, in hundredths of a decibel, as a linear gain. */
float GainOfVolume(LONG volume);
/* The 3D gains of a source at offset (from the listener, in world
 * coordinates) for a listener facing front with top up: distance
 * attenuation times the pan, per channel. */
void GainsOf3D(const float offset[3], const float front[3], const float top[3],
               float minDistance, float maxDistance, float rolloff,
               float* left, float* right);
/* A mixer fader value, 0 to 65535, as a linear gain. */
float GainOfFader(DWORD value);
} // namespace audio_sdl_detail
#endif

#endif /* LEGORRR_LIBRARY_AUDIO_SDL_H */
