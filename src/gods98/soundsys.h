// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 sound layer (original translation unit G98-021, finding 505;
 * probably Sound.c): the sound front end and CD track player, the RIFF
 * wave readers and the MCI "cdaudio" commands.
 *
 * C-compatible header (ADR 0005). soundsys.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). The CD functions are also declared
 * in gods98/media.hpp; src/game/test_c_boundaries.cpp checks that the two
 * agree. The wave readers use the Win32 types of gods98/winmm.h, and only
 * sound.cpp calls them from C++, through this header. */
#ifndef LEGORRR_GODS98_SOUNDSYS_H
#define LEGORRR_GODS98_SOUNDSYS_H

#include "winmm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SoundCDStopCallback)(void);

/* Sound_Initialise (FUN_00488b50): sound is on unless nosound; with sound
 * on, starts 3D sound (audio_system_init) on the main window and returns
 * whether it started. Sets the CD loop flag (-1). */
int media_cd_audio_device_state_update(int nosound);

/* Sound_IsInitialised (FUN_00488b90): sound on and 3D sound running. */
int media_cd_audio_playback_available(void);

/* Sound_PlayCDTrack (FUN_00488bb0): with sound on, remember track and the
 * callback, set looping for mode 1, and play the track from the start of
 * the disc (media_cd_audio_device_open). */
void media_cd_audio_track_start(int track, int mode,
                                SoundCDStopCallback callback);

/* Sound_StopCD (FUN_00488bf0): with sound on, stop and close the CD. */
void media_cd_audio_stop_and_close_if_active(void);

/* Sound_Update (FUN_00488c00): with sound on and cdtrack set, poll the
 * track at most every 4,000 ms; when it has ended, replay it if looping
 * and call the callback. */
void media_cd_audio_watchdog_tick(int cdtrack);

/* WaveOpenFile over memory (FUN_00488c70). Returns 0 or a 0xe0xx code;
 * on success *phmmioIn is left after the 'fmt ' chunk and *ppwfxInfo is
 * a GlobalAlloc block (cbSize and any extra format bytes included). */
int sound_wave_memory_format_read(void* fileData, LONG fileSize,
                                  HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
                                  MMCKINFO* pckInRIFF);

/* FUN_00488e70: the nAvgBytesPerSec of a wave file's 'fmt ' chunk, or 0. */
DWORD sound_wave_file_average_bytes_per_second_read(const char* pszFileName);

/* WaveOpenFile (FUN_00488f10): as the memory variant, from a file. */
int sound_wave_file_open_read_chunks(const char* pszFileName,
                                     HMMIO* phmmioIn,
                                     WAVEFORMATEX** ppwfxInfo,
                                     MMCKINFO* pckInRIFF);

/* WaveStartDataRead (FUN_004890c0): descend into the 'data' chunk;
 * returns mmioDescend's result. */
int sound_wave_data_chunk_open(HMMIO* phmmioIn, MMCKINFO* pckIn,
                               MMCKINFO* pckInRIFF);

/* WaveReadFile (FUN_00489100): read up to cbRead bytes of the data chunk;
 * pckIn->cksize counts down the bytes left. */
int sound_wave_data_read(HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
                         MMCKINFO* pckIn, UINT* cbActualRead);

/* WaveCloseReadFile (FUN_004891d0): free the format and close the handle;
 * both are cleared. Returns 0. */
int sound_wave_handle_and_buffer_release(HMMIO* phmmio,
                                         WAVEFORMATEX** ppwfxSrc);

/* Restart_CDTrack (FUN_00489210): MCI "play cdaudio from <track + 1>". */
int media_cd_audio_track_play_from(int track);

/* ReportCDError (FUN_00489260): the last MCI error's text. */
void media_cd_audio_error_text_get(void);

/* Status_CDTrack (FUN_00489280): whether the disc is playing track (0-based)
 * or an earlier one. */
int media_cd_audio_track_completion_poll(int track);

/* Play_CDTrack (FUN_00489330): open the CD device (TMSF time format) and
 * play from track. */
int media_cd_audio_device_open(int track);

/* Stop_CDTrack (FUN_004893a0): stop the disc and close the device. */
int media_cd_audio_stop_and_close(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SOUNDSYS_H */
