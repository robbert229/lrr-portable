// SPDX-License-Identifier: LGPL-3.0-or-later
/* The Win32 subset that the gods98 sound layer (soundsys.c) calls: WinMM's
 * mmio, MCI and timeGetTime, KERNEL32's GlobalAlloc/GlobalFree and USER32's
 * wsprintfA.
 *
 * C-compatible header (ADR 0005). The Windows builds (the matching build and
 * the MinGW reference build) take the SDK's own declarations, so every call
 * goes through the import table as in the original (`call [__imp__...]`).
 * The native port has no WinMM: this header declares the same names and
 * layouts, and winmm_port.cpp emulates them (mmio over memory and the port's
 * loose-file layer, MCI as a missing "cdaudio" device). Shared C++ headers do
 * not include this file (finding 506). */
#ifndef LEGORRR_GODS98_WINMM_H
#define LEGORRR_GODS98_WINMM_H

#if defined(_WIN32)

#include <windows.h>
#include <mmsystem.h>

#else /* native port */

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus) && defined(LEGORRR_BACKEND_MINIWIN)
/* The port's C++ code sees miniwin's base types; reuse them. */
#include <miniwin/windows.h>
#else
typedef uint8_t BYTE;
typedef unsigned short WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef unsigned int UINT;
typedef int BOOL;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef void* HANDLE;
typedef struct HWND__* HWND;
#define WINAPI
#define TRUE 1
#define FALSE 0
#endif

typedef HANDLE HGLOBAL;
typedef char* HPSTR;
typedef DWORD FOURCC;
typedef UINT MMRESULT;
typedef DWORD MCIERROR;
typedef struct HMMIO__* HMMIO;

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

#ifdef __cplusplus
extern "C" {
#endif

HGLOBAL GlobalAlloc(UINT flags, size_t bytes);
HGLOBAL GlobalFree(HGLOBAL memory);
int wsprintfA(LPSTR output, LPCSTR format, ...);
DWORD timeGetTime(void);

HMMIO mmioOpenA(LPSTR name, LPMMIOINFO info, DWORD flags);
MMRESULT mmioClose(HMMIO hmmio, UINT flags);
LONG mmioRead(HMMIO hmmio, HPSTR buffer, LONG bytes);
LONG mmioSeek(HMMIO hmmio, LONG offset, int origin);
MMRESULT mmioGetInfo(HMMIO hmmio, LPMMIOINFO info, UINT flags);
MMRESULT mmioSetInfo(HMMIO hmmio, const MMIOINFO* info, UINT flags);
MMRESULT mmioAdvance(HMMIO hmmio, LPMMIOINFO info, UINT flags);
MMRESULT mmioDescend(HMMIO hmmio, LPMMCKINFO chunk,
                     const MMCKINFO* parent, UINT flags);
MMRESULT mmioAscend(HMMIO hmmio, LPMMCKINFO chunk, UINT flags);

MCIERROR mciSendStringA(LPCSTR command, LPSTR result, UINT capacity,
                        HWND callback);
BOOL mciGetErrorStringA(MCIERROR error, LPSTR text, UINT capacity);

/* Port-only test seam: replaces the missing "cdaudio" device. The hook gets
 * each command string and the caller's result buffer and returns the MCI
 * error code; NULL restores the default (every command fails with
 * MCIERR_INVALID_DEVICE_NAME). The time hook replaces timeGetTime. */
typedef MCIERROR (*WinmmPortMciHook)(LPCSTR command, LPSTR result,
                                     UINT capacity);
void winmm_port_mci_hook_set(WinmmPortMciHook hook);
typedef DWORD (*WinmmPortTimeHook)(void);
void winmm_port_time_hook_set(WinmmPortTimeHook hook);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* LEGORRR_GODS98_WINMM_H */
