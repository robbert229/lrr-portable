// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 sound layer — original translation unit G98-021 (text
 * 00488b50-004893f0, .data 004ab6d0-004ab783, .bss 00544868-00544a20,
 * common 00576500; findings 505 and 512). The file is probably gods98's
 * Sound.c, but no name is evidenced in the binary, and gods98/sound.h is
 * already the C boundary header of the port's sound.cpp, so the stem is
 * soundsys.
 *
 * Rebuilt in the original's shape (issues #409 and #410, ADR 0005). The
 * unit has three parts, in original order:
 *
 *  - the sound front end: Sound_Initialise, Sound_IsInitialised and the CD
 *    track player (play, stop and the four-second completion poll);
 *  - the RIFF wave readers, the DirectX SDK's wave.c routines (WaveOpenFile,
 *    WaveStartDataRead, WaveReadFile, WaveCloseReadFile) with an in-memory
 *    variant and an average-byte-rate probe. Only 3DSound.c calls them;
 *  - the MCI "cdaudio" commands that the CD track player uses.
 *
 * Win32 calls go through gods98/winmm.h: the Windows builds call the
 * imports, and the native port uses winmm_port.cpp's emulation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winmm.h"
#include "soundsys.h"
#include "audio.h"
#include "platform/window.h"
#include "crt/vc5names.h"

/* WaveOpenFile's error codes (DirectX SDK wave.h). */
#define WAVE_ER_MEM 0xe000
#define WAVE_ER_CANNOTOPEN 0xe100
#define WAVE_ER_NOTWAVEFILE 0xe101
#define WAVE_ER_CANNOTREAD 0xe102
#define WAVE_ER_CORRUPTWAVEFILE 0xe103

#define SOUND_MCI_RETURN_SIZE 200

/* The unit's .bss, one struct. The CD player's state and the last MCI
 * error are the only fields the code reads; the rest keep the original's
 * layout. MSVC gives the object an 8-aligned .bss because of the 400-byte
 * block, which is why it starts 4 bytes before the first field used. */
typedef struct SoundGlobs {
    int reserved_000;                /* 00544868: never referenced */
    int useSound;                    /* 0054486c: !nosound */
    unsigned int reserved_008[100];  /* 00544870: never referenced */
    int currTrack;                   /* 00544a00: the CD track playing */
    int loopCDTrack;                 /* 00544a04: replay when it ends */
    SoundCDStopCallback CDStopCallback; /* 00544a08: called when it ends */
    int reserved_1a4;                /* 00544a0c: never referenced */
    DWORD lastUpdate;                /* 00544a10: timeGetTime of last poll */
    int reserved_1ac[2];             /* 00544a14: never referenced */
    MCIERROR mciErr;                 /* 00544a1c: the last MCI result */
} SoundGlobs;

/* The unit's initialised .data. Nothing in the binary reads it. */
// GLOBAL: LEGORR 0x004ab6d0
int sound_unused_004ab6d0 = 1;
// GLOBAL: LEGORR 0x004ab6d4
int sound_unused_004ab6d4 = 1;
// GLOBAL: LEGORR 0x004ab6d8
char sound_cd_device_name[8] = "cdaudio";

// GLOBAL: LEGORR 0x00544868
static SoundGlobs soundGlobs;

/* The MCI result text, a common symbol. */
// GLOBAL: LEGORR 0x00576500
char sound_mci_return[SOUND_MCI_RETURN_SIZE];

/* Sound_Initialise: with sound on, start 3D sound on the main window. */
// FUNCTION: LEGORR 0x00488b50
int media_cd_audio_device_state_update(int nosound)
{
    soundGlobs.useSound = !nosound;
    soundGlobs.loopCDTrack = -1;
    if (nosound) return 1;
    return audio_system_init(g_platform_cursor_window_handle) != 0;
}

/* Sound_IsInitialised. */
// FUNCTION: LEGORR 0x00488b90
int media_cd_audio_playback_available(void)
{
    if (soundGlobs.useSound && s_bank_sound_on) return 1;
    return 0;
}

/* Sound_PlayCDTrack: mode 1 loops the track. The callback runs when the
 * poll finds the track over. */
// FUNCTION: LEGORR 0x00488bb0
void media_cd_audio_track_start(int track, int mode,
                                SoundCDStopCallback callback)
{
    if (soundGlobs.useSound) {
        if (mode == 1) soundGlobs.loopCDTrack = 1;
        soundGlobs.currTrack = track;
        soundGlobs.CDStopCallback = callback;
        media_cd_audio_device_open(track);
    }
}

/* Sound_StopCD. */
// FUNCTION: LEGORR 0x00488bf0
void media_cd_audio_stop_and_close_if_active(void)
{
    if (soundGlobs.useSound) media_cd_audio_stop_and_close();
}

/* Sound_Update: at most every four seconds, ask MCI whether the track is
 * still playing; if not, replay it when looping and call the callback. */
// FUNCTION: LEGORR 0x00488c00
void media_cd_audio_watchdog_tick(int cdtrack)
{
    DWORD currTime;

    if (soundGlobs.useSound) {
        currTime = timeGetTime();
        if (currTime > soundGlobs.lastUpdate + 4000) {
            if (cdtrack) {
                if (!media_cd_audio_track_completion_poll(soundGlobs.currTrack)) {
                    if (soundGlobs.loopCDTrack)
                        media_cd_audio_track_play_from(soundGlobs.currTrack);
                    if (soundGlobs.CDStopCallback != NULL)
                        soundGlobs.CDStopCallback();
                }
            }
            soundGlobs.lastUpdate = currTime;
        }
    }
}

/* WaveOpenFile, reading a wave file already in memory through mmio's
 * memory I/O procedure. Leaves the handle in the 'RIFF' chunk after 'fmt '
 * and returns the format block from GlobalAlloc. */
// FUNCTION: LEGORR 0x00488c70
int sound_wave_memory_format_read(void* fileData, LONG fileSize,
                                  HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
                                  MMCKINFO* pckInRIFF)
{
    HMMIO hmmioIn;
    MMCKINFO ckIn;
    PCMWAVEFORMAT pcmWaveFormat;
    WORD cbExtraAlloc;
    int nError;
    MMIOINFO mmioInfo;

    *ppwfxInfo = NULL;
    nError = 0;
    hmmioIn = NULL;

    memset(&mmioInfo, 0, sizeof(mmioInfo));
    mmioInfo.fccIOProc = FOURCC_MEM;
    mmioInfo.pchBuffer = (HPSTR)fileData;
    mmioInfo.cchBuffer = fileSize;

    if ((hmmioIn = mmioOpenA(NULL, &mmioInfo, MMIO_ALLOCBUF | MMIO_READ)) == NULL) {
        nError = WAVE_ER_CANNOTOPEN;
        goto ERROR_READING_WAVE;
    }
    if ((nError = (int)mmioDescend(hmmioIn, pckInRIFF, NULL, 0)) != 0)
        goto ERROR_READING_WAVE;
    if (pckInRIFF->ckid != FOURCC_RIFF ||
        pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        nError = WAVE_ER_NOTWAVEFILE;
        goto ERROR_READING_WAVE;
    }
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (int)mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
        goto ERROR_READING_WAVE;
    if (ckIn.cksize < (long)sizeof(PCMWAVEFORMAT)) {
        nError = WAVE_ER_NOTWAVEFILE;
        goto ERROR_READING_WAVE;
    }
    if (mmioRead(hmmioIn, (HPSTR)&pcmWaveFormat, (long)sizeof(pcmWaveFormat)) != (long)sizeof(pcmWaveFormat)) {
        nError = WAVE_ER_CANNOTREAD;
        goto ERROR_READING_WAVE;
    }
    if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
        cbExtraAlloc = 0;
    else {
        if (mmioRead(hmmioIn, (HPSTR)&cbExtraAlloc, (long)sizeof(cbExtraAlloc)) != (long)sizeof(cbExtraAlloc)) {
            nError = WAVE_ER_CANNOTREAD;
            goto ERROR_READING_WAVE;
        }
    }
    if ((*ppwfxInfo = (WAVEFORMATEX*)GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX) + cbExtraAlloc)) == NULL) {
        nError = WAVE_ER_MEM;
        goto ERROR_READING_WAVE;
    }
    memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
    (*ppwfxInfo)->cbSize = cbExtraAlloc;
    if (cbExtraAlloc != 0) {
        if (mmioRead(hmmioIn, (HPSTR)(((BYTE*)&((*ppwfxInfo)->cbSize)) + sizeof(cbExtraAlloc)), (long)cbExtraAlloc) != (long)cbExtraAlloc) {
            nError = WAVE_ER_NOTWAVEFILE;
            goto ERROR_READING_WAVE;
        }
    }
    if ((nError = mmioAscend(hmmioIn, &ckIn, 0)) != 0)
        goto ERROR_READING_WAVE;
    goto TEMPCLEANUP;

ERROR_READING_WAVE:
    if (*ppwfxInfo != NULL) {
        GlobalFree(*ppwfxInfo);
        *ppwfxInfo = NULL;
    }
    if (hmmioIn != NULL) {
        mmioClose(hmmioIn, 0);
        hmmioIn = NULL;
    }

TEMPCLEANUP:
    *phmmioIn = hmmioIn;
    return nError;
}

/* The 'fmt ' chunk's average byte rate of a wave file, or 0. */
// FUNCTION: LEGORR 0x00488e70
DWORD sound_wave_file_average_bytes_per_second_read(const char* pszFileName)
{
    HMMIO hmmioIn;
    MMCKINFO ckInRiff;
    MMCKINFO ckIn;
    PCMWAVEFORMAT pcmWaveFormat;
    DWORD avgBytesPerSec;

    avgBytesPerSec = 0;
    if ((hmmioIn = mmioOpenA((LPSTR)pszFileName, NULL, MMIO_READ)) != NULL) {
        if (mmioDescend(hmmioIn, &ckInRiff, NULL, 0) == 0 &&
            ckInRiff.ckid == FOURCC_RIFF &&
            ckInRiff.fccType == mmioFOURCC('W', 'A', 'V', 'E')) {
            ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
            if (mmioDescend(hmmioIn, &ckIn, &ckInRiff, MMIO_FINDCHUNK) == 0 &&
                ckIn.cksize >= (long)sizeof(PCMWAVEFORMAT)) {
                if (mmioRead(hmmioIn, (HPSTR)&pcmWaveFormat, (long)sizeof(pcmWaveFormat)) == (long)sizeof(pcmWaveFormat))
                    avgBytesPerSec = pcmWaveFormat.wf.nAvgBytesPerSec;
            }
        }
    }
    if (hmmioIn != NULL) mmioClose(hmmioIn, 0);
    return avgBytesPerSec;
}

/* WaveOpenFile (DirectX SDK wave.c), from a file. */
// FUNCTION: LEGORR 0x00488f10
int sound_wave_file_open_read_chunks(const char* pszFileName,
                                     HMMIO* phmmioIn,
                                     WAVEFORMATEX** ppwfxInfo,
                                     MMCKINFO* pckInRIFF)
{
    HMMIO hmmioIn;
    MMCKINFO ckIn;
    PCMWAVEFORMAT pcmWaveFormat;
    WORD cbExtraAlloc;
    int nError;

    *ppwfxInfo = NULL;
    nError = 0;
    hmmioIn = NULL;

    if ((hmmioIn = mmioOpenA((LPSTR)pszFileName, NULL, MMIO_ALLOCBUF | MMIO_READ)) == NULL) {
        nError = WAVE_ER_CANNOTOPEN;
        goto ERROR_READING_WAVE;
    }
    if ((nError = (int)mmioDescend(hmmioIn, pckInRIFF, NULL, 0)) != 0)
        goto ERROR_READING_WAVE;
    if (pckInRIFF->ckid != FOURCC_RIFF ||
        pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        nError = WAVE_ER_NOTWAVEFILE;
        goto ERROR_READING_WAVE;
    }
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (int)mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
        goto ERROR_READING_WAVE;
    if (ckIn.cksize < (long)sizeof(PCMWAVEFORMAT)) {
        nError = WAVE_ER_NOTWAVEFILE;
        goto ERROR_READING_WAVE;
    }
    if (mmioRead(hmmioIn, (HPSTR)&pcmWaveFormat, (long)sizeof(pcmWaveFormat)) != (long)sizeof(pcmWaveFormat)) {
        nError = WAVE_ER_CANNOTREAD;
        goto ERROR_READING_WAVE;
    }
    if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
        cbExtraAlloc = 0;
    else {
        if (mmioRead(hmmioIn, (HPSTR)&cbExtraAlloc, (long)sizeof(cbExtraAlloc)) != (long)sizeof(cbExtraAlloc)) {
            nError = WAVE_ER_CANNOTREAD;
            goto ERROR_READING_WAVE;
        }
    }
    if ((*ppwfxInfo = (WAVEFORMATEX*)GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX) + cbExtraAlloc)) == NULL) {
        nError = WAVE_ER_MEM;
        goto ERROR_READING_WAVE;
    }
    memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
    (*ppwfxInfo)->cbSize = cbExtraAlloc;
    if (cbExtraAlloc != 0) {
        if (mmioRead(hmmioIn, (HPSTR)(((BYTE*)&((*ppwfxInfo)->cbSize)) + sizeof(cbExtraAlloc)), (long)cbExtraAlloc) != (long)cbExtraAlloc) {
            nError = WAVE_ER_NOTWAVEFILE;
            goto ERROR_READING_WAVE;
        }
    }
    if ((nError = mmioAscend(hmmioIn, &ckIn, 0)) != 0)
        goto ERROR_READING_WAVE;
    goto TEMPCLEANUP;

ERROR_READING_WAVE:
    if (*ppwfxInfo != NULL) {
        GlobalFree(*ppwfxInfo);
        *ppwfxInfo = NULL;
    }
    if (hmmioIn != NULL) {
        mmioClose(hmmioIn, 0);
        hmmioIn = NULL;
    }

TEMPCLEANUP:
    *phmmioIn = hmmioIn;
    return nError;
}

/* WaveStartDataRead: seek back to the start of the RIFF form and descend
 * into its 'data' chunk. */
// FUNCTION: LEGORR 0x004890c0
int sound_wave_data_chunk_open(HMMIO* phmmioIn, MMCKINFO* pckIn,
                               MMCKINFO* pckInRIFF)
{
    mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
    return mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK);
}

/* WaveReadFile: copy up to cbRead bytes of the data chunk through mmio's
 * buffer. pckIn->cksize counts down what is left. The SDK's precedence
 * slip (nError = mmioGetInfo(...) != 0) is the original's too. */
// FUNCTION: LEGORR 0x00489100
int sound_wave_data_read(HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
                         MMCKINFO* pckIn, UINT* cbActualRead)
{
    MMIOINFO mmioinfoIn;
    int nError;
    UINT cT, cbDataIn;

    if ((nError = mmioGetInfo(hmmioIn, &mmioinfoIn, 0) != 0) != 0)
        goto ERROR_CANNOT_READ;
    cbDataIn = cbRead;
    if (cbDataIn > pckIn->cksize)
        cbDataIn = pckIn->cksize;
    pckIn->cksize -= cbDataIn;
    for (cT = 0; cT < cbDataIn; cT++) {
        if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead) {
            if ((nError = mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ)) != 0)
                goto ERROR_CANNOT_READ;
            if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead) {
                nError = WAVE_ER_CORRUPTWAVEFILE;
                goto ERROR_CANNOT_READ;
            }
        }
        *((BYTE*)pbDest + cT) = (BYTE)*mmioinfoIn.pchNext++;
    }
    if ((nError = mmioSetInfo(hmmioIn, &mmioinfoIn, 0)) != 0)
        goto ERROR_CANNOT_READ;
    *cbActualRead = cbDataIn;
    goto FINISHED_READING;

ERROR_CANNOT_READ:
    *cbActualRead = 0;

FINISHED_READING:
    return nError;
}

/* WaveCloseReadFile. */
// FUNCTION: LEGORR 0x004891d0
int sound_wave_handle_and_buffer_release(HMMIO* phmmio,
                                         WAVEFORMATEX** ppwfxSrc)
{
    if (*ppwfxSrc != NULL) {
        GlobalFree(*ppwfxSrc);
        *ppwfxSrc = NULL;
    }
    if (*phmmio != NULL) {
        mmioClose(*phmmio, 0);
        *phmmio = NULL;
    }
    return 0;
}

/* Restart_CDTrack: play from the start of track (0-based) to the end of the
 * disc. */
// FUNCTION: LEGORR 0x00489210
int media_cd_audio_track_play_from(int track)
{
    char chText[100];

    // STRING: LEGORR 0x004ab6e0
    wsprintfA(chText, "play cdaudio from %i", track + 1);
    if ((soundGlobs.mciErr = mciSendStringA(chText, sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL)) != 0) {
        media_cd_audio_error_text_get();
        return 0;
    }
    return 1;
}

/* ReportCDError: the text of the last MCI error, into the result buffer. */
// FUNCTION: LEGORR 0x00489260
void media_cd_audio_error_text_get(void)
{
    mciGetErrorStringA(soundGlobs.mciErr, sound_mci_return, SOUND_MCI_RETURN_SIZE);
}

/* Status_CDTrack: whether the disc is still playing track or an earlier
 * one. A stopped disc shares the error path of a later track: the original
 * has one ReportCDError block that both tests reach. */
// FUNCTION: LEGORR 0x00489280
int media_cd_audio_track_completion_poll(int track)
{
    char chText[100];

    track++;
    // STRING: LEGORR 0x004ab720
    sprintf(chText, "status cdaudio mode");
    soundGlobs.mciErr = mciSendStringA(chText, sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL);
    // STRING: LEGORR 0x004ab718
    if (_strcmpi(sound_mci_return, "stopped") == 0) goto not_playing;
    // STRING: LEGORR 0x004ab6f8
    sprintf(chText, "status cdaudio current track");
    soundGlobs.mciErr = mciSendStringA(chText, sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL);
    if (atoi(sound_mci_return) > track) {
not_playing:
        media_cd_audio_error_text_get();
        return 0;
    }
    return 1;
}

/* Play_CDTrack: open the CD device with track/minute/second/frame times
 * and play from track. Every failure reaches one ReportCDError, as in the
 * original: each command runs only while the last one succeeded. */
// FUNCTION: LEGORR 0x00489330
int media_cd_audio_device_open(int track)
{
    // STRING: LEGORR 0x004ab754
    if ((soundGlobs.mciErr = mciSendStringA("open cdaudio", sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL)) == 0) {
        // STRING: LEGORR 0x004ab734
        if ((soundGlobs.mciErr = mciSendStringA("set cdaudio time format tmsf", sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL)) == 0)
            media_cd_audio_track_play_from(track);
    }
    if (soundGlobs.mciErr != 0) {
        media_cd_audio_error_text_get();
        return 0;
    }
    return 1;
}

/* Stop_CDTrack: stop the disc and close the device (one error path, as
 * Play_CDTrack). */
// FUNCTION: LEGORR 0x004893a0
int media_cd_audio_stop_and_close(void)
{
    // STRING: LEGORR 0x004ab774
    if ((soundGlobs.mciErr = mciSendStringA("stop cdaudio", sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL)) == 0)
        // STRING: LEGORR 0x004ab764
        soundGlobs.mciErr = mciSendStringA("close cdaudio", sound_mci_return, SOUND_MCI_RETURN_SIZE, NULL);
    if (soundGlobs.mciErr != 0) {
        media_cd_audio_error_text_get();
        return 0;
    }
    return 1;
}
