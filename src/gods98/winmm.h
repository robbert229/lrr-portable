// SPDX-License-Identifier: LGPL-3.0-or-later
/* The Win32 subset outside miniwin's scope that the units use: WinMM's
 * mmio, MCI and mixer types, KERNEL32's GlobalAlloc/GlobalFree and
 * USER32's wsprintfA, with the layouts the units read. The develop branch
 * has no WinMM. The audio library (library/audio_sdl.h) replaces the mmio,
 * MCI and mixer calls at the units' `// [library:audio]` call sites. */
#ifndef LEGORRR_GODS98_WINMM_H
#define LEGORRR_GODS98_WINMM_H

#include <stddef.h>
#include <stdint.h>

#include <miniwin/windows.h>

typedef HANDLE HGLOBAL;
typedef char* HPSTR;
typedef DWORD FOURCC;
typedef UINT MMRESULT;
typedef DWORD MCIERROR;
typedef struct HMMIO__* HMMIO;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t UINT_PTR;

#define GMEM_FIXED 0x0000

#define mmioFOURCC(ch0, ch1, ch2, ch3)                                  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |                   \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#define FOURCC_RIFF mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST mmioFOURCC('L', 'I', 'S', 'T')
#define FOURCC_MEM mmioFOURCC('M', 'E', 'M', ' ')

#define MMIO_READ 0x00000000
#define MMIO_ALLOCBUF 0x00010000
#define MMIO_FINDCHUNK 0x0010
#define MMIO_FINDRIFF 0x0020
#define MMIO_FINDLIST 0x0040

#define MMSYSERR_NOERROR 0
#define MMSYSERR_INVALHANDLE 5
#define MMIOERR_CANNOTOPEN 259
#define MMIOERR_CANNOTREAD 260
#define MMIOERR_CANNOTSEEK 262
#define MMIOERR_CHUNKNOTFOUND 265
#define MCIERR_INVALID_DEVICE_NAME 263

#define WAVE_FORMAT_PCM 1

#pragma pack(push, 1)
typedef struct _MMIOINFO {
    DWORD dwFlags;
    FOURCC fccIOProc;
    void* pIOProc;
    UINT wErrorRet;
    HANDLE htask;
    LONG cchBuffer;
    HPSTR pchBuffer;
    HPSTR pchNext;
    HPSTR pchEndRead;
    HPSTR pchEndWrite;
    LONG lBufOffset;
    LONG lDiskOffset;
    DWORD adwInfo[3];
    DWORD dwReserved1;
    DWORD dwReserved2;
    HMMIO hmmio;
} MMIOINFO, *LPMMIOINFO;

typedef struct _MMCKINFO {
    FOURCC ckid;
    DWORD cksize;
    FOURCC fccType;
    DWORD dwDataOffset;
    DWORD dwFlags;
} MMCKINFO, *LPMMCKINFO;

typedef struct waveformat_tag {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
} WAVEFORMAT;

typedef struct pcmwaveformat_tag {
    WAVEFORMAT wf;
    WORD wBitsPerSample;
} PCMWAVEFORMAT;

typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;
#pragma pack(pop)

/* The mixer (mmsystem.h). */
typedef struct HMIXER__* HMIXER;
typedef struct HMIXEROBJ__* HMIXEROBJ;

#define MAXPNAMELEN 32
#define MIXER_SHORT_NAME_CHARS 16
#define MIXER_LONG_NAME_CHARS 64

#define MIXER_GETLINEINFOF_DESTINATION 0x00000000L
#define MIXER_GETLINEINFOF_SOURCE 0x00000001L
#define MIXER_GETLINECONTROLSF_ALL 0x00000000L
#define MIXER_GETCONTROLDETAILSF_VALUE 0x00000000L
#define MIXER_SETCONTROLDETAILSF_VALUE 0x00000000L
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC 0x00001005L
#define MIXERCONTROL_CT_CLASS_FADER 0x50000000L
#define MIXERCONTROL_CONTROLTYPE_VOLUME 0x50030001L

typedef struct tagMIXERCAPSA {
    WORD wMid;
    WORD wPid;
    UINT vDriverVersion;
    char szPname[MAXPNAMELEN];
    DWORD fdwSupport;
    DWORD cDestinations;
} MIXERCAPSA;

typedef struct tagMIXERLINEA {
    DWORD cbStruct;
    DWORD dwDestination;
    DWORD dwSource;
    DWORD dwLineID;
    DWORD fdwLine;
    DWORD_PTR dwUser;
    DWORD dwComponentType;
    DWORD cChannels;
    DWORD cConnections;
    DWORD cControls;
    char szShortName[MIXER_SHORT_NAME_CHARS];
    char szName[MIXER_LONG_NAME_CHARS];
    struct {
        DWORD dwType;
        DWORD dwDeviceID;
        WORD wMid;
        WORD wPid;
        UINT vDriverVersion;
        char szPname[MAXPNAMELEN];
    } Target;
} MIXERLINEA;

typedef struct tagMIXERCONTROLA {
    DWORD cbStruct;
    DWORD dwControlID;
    DWORD dwControlType;
    DWORD fdwControl;
    DWORD cMultipleItems;
    char szShortName[MIXER_SHORT_NAME_CHARS];
    char szName[MIXER_LONG_NAME_CHARS];
    struct {
        DWORD dwMinimum;
        DWORD dwMaximum;
        DWORD dwReserved[4];
    } Bounds;
    DWORD Metrics[6];
} MIXERCONTROLA;

typedef struct tagMIXERLINECONTROLSA {
    DWORD cbStruct;
    DWORD dwLineID;
    DWORD dwControl;
    DWORD cControls;
    DWORD cbmxctrl;
    MIXERCONTROLA* pamxctrl;
} MIXERLINECONTROLSA;

typedef struct tMIXERCONTROLDETAILS {
    DWORD cbStruct;
    DWORD dwControlID;
    DWORD cChannels;
    DWORD cMultipleItems;
    DWORD cbDetails;
    void* paDetails;
} MIXERCONTROLDETAILS;

#ifdef __cplusplus
extern "C" {
#endif

/* KERNEL32 and USER32, in the seam (platform/native/seam_stubs.cpp) under
 * names of their own, so that on Windows they do not collide with the
 * system's, which SDL and the C library link. */
HANDLE seam_create_mutex(void* attributes, BOOL initialOwner, LPCSTR name);
DWORD seam_get_last_error(void);
LPSTR seam_get_command_line(void);

HGLOBAL seam_global_alloc(UINT flags, size_t bytes);
HGLOBAL seam_global_free(HGLOBAL memory);
int seam_wsprintf(LPSTR output, LPCSTR format, ...);
#define CreateMutexA seam_create_mutex
#define GetLastError seam_get_last_error
#define GetCommandLineA seam_get_command_line
#define GlobalAlloc seam_global_alloc
#define GlobalFree seam_global_free
#define wsprintfA seam_wsprintf

#ifdef __cplusplus
}
#endif


#endif /* LEGORRR_GODS98_WINMM_H */
