// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port-only: the native port's stand-ins for the Win32 entries that the
 * gods98 sound layer (soundsys.c) imports, declared in gods98/winmm.h. The
 * Windows builds link the real WINMM, KERNEL32 and USER32 instead.
 *
 * - mmio reads a whole file (or the caller's memory, for the 'MEM ' I/O
 *   procedure) into one buffer, so mmioGetInfo exposes all of it and
 *   mmioAdvance has nothing more to read.
 * - MCI has no "cdaudio" device: every command fails with
 *   MCIERR_INVALID_DEVICE_NAME unless a test installs a hook.
 * - timeGetTime is the platform's millisecond clock. */
#if !defined(_WIN32)

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/platform.h"
#include "winmm.h"

struct HMMIO__ {
    char* data;
    LONG length;
    LONG pos;
    int owned;
};

static WinmmPortMciHook s_mci_hook;
static WinmmPortTimeHook s_time_hook;

extern "C" {

void winmm_port_mci_hook_set(WinmmPortMciHook hook) { s_mci_hook = hook; }
void winmm_port_time_hook_set(WinmmPortTimeHook hook) { s_time_hook = hook; }

HGLOBAL GlobalAlloc(UINT flags, size_t bytes)
{
    (void)flags;
    return malloc(bytes != 0 ? bytes : 1);
}

HGLOBAL GlobalFree(HGLOBAL memory)
{
    free(memory);
    return NULL;
}

/* USER32's wsprintfA writes at most 1,024 characters. */
int wsprintfA(LPSTR output, LPCSTR format, ...)
{
    va_list args;
    int written;
    va_start(args, format);
    written = vsnprintf(output, 1024, format, args);
    va_end(args);
    if (written > 1023) written = 1023;
    return written;
}

DWORD timeGetTime(void)
{
    if (s_time_hook != NULL) return s_time_hook();
    return (DWORD)platform_time_milliseconds();
}

HMMIO mmioOpenA(LPSTR name, LPMMIOINFO info, DWORD flags)
{
    HMMIO hmmio;
    (void)flags;
    if (info != NULL) info->wErrorRet = 0;
    hmmio = (HMMIO)calloc(1, sizeof(*hmmio));
    if (hmmio == NULL) return NULL;
    if (info != NULL && info->fccIOProc == FOURCC_MEM) {
        hmmio->data = info->pchBuffer;
        hmmio->length = info->cchBuffer >= 0 ? info->cchBuffer : 0;
        return hmmio;
    }
    if (name != NULL) {
        FILE* file = fopen(name, "rb");
        long length = -1;
        if (file != NULL && fseek(file, 0, SEEK_END) == 0) {
            length = ftell(file);
            if (length >= 0 && length <= 0x7fffffffL &&
                fseek(file, 0, SEEK_SET) == 0) {
                hmmio->data = (char*)malloc(length != 0 ? (size_t)length : 1u);
                if (hmmio->data != NULL &&
                    fread(hmmio->data, 1, (size_t)length, file) == (size_t)length) {
                    fclose(file);
                    hmmio->length = (LONG)length;
                    hmmio->owned = 1;
                    return hmmio;
                }
                free(hmmio->data);
            }
        }
        if (file != NULL) fclose(file);
    }
    free(hmmio);
    if (info != NULL) info->wErrorRet = MMIOERR_CANNOTOPEN;
    return NULL;
}

MMRESULT mmioClose(HMMIO hmmio, UINT flags)
{
    (void)flags;
    if (hmmio == NULL) return MMSYSERR_INVALHANDLE;
    if (hmmio->owned) free(hmmio->data);
    free(hmmio);
    return MMSYSERR_NOERROR;
}

LONG mmioRead(HMMIO hmmio, HPSTR buffer, LONG bytes)
{
    LONG left;
    if (hmmio == NULL || bytes < 0) return -1;
    left = hmmio->length - hmmio->pos;
    if (bytes > left) bytes = left;
    if (bytes > 0) {
        memcpy(buffer, hmmio->data + hmmio->pos, (size_t)bytes);
        hmmio->pos += bytes;
    }
    return bytes;
}

LONG mmioSeek(HMMIO hmmio, LONG offset, int origin)
{
    int64_t target;
    if (hmmio == NULL) return -1;
    if (origin == SEEK_SET) target = offset;
    else if (origin == SEEK_CUR) target = (int64_t)hmmio->pos + offset;
    else if (origin == SEEK_END) target = (int64_t)hmmio->length + offset;
    else return -1;
    if (target < 0 || target > hmmio->length) return -1;
    hmmio->pos = (LONG)target;
    return hmmio->pos;
}

/* The whole stream is the buffer. */
MMRESULT mmioGetInfo(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    (void)flags;
    if (hmmio == NULL || info == NULL) return MMSYSERR_INVALHANDLE;
    memset(info, 0, sizeof(*info));
    info->cchBuffer = hmmio->length;
    info->pchBuffer = hmmio->data;
    info->pchNext = hmmio->data + hmmio->pos;
    info->pchEndRead = hmmio->data + hmmio->length;
    info->pchEndWrite = hmmio->data + hmmio->length;
    info->lDiskOffset = hmmio->length;
    info->hmmio = hmmio;
    return MMSYSERR_NOERROR;
}

MMRESULT mmioSetInfo(HMMIO hmmio, const MMIOINFO* info, UINT flags)
{
    (void)flags;
    if (hmmio == NULL || info == NULL) return MMSYSERR_INVALHANDLE;
    hmmio->pos = (LONG)(info->pchNext - hmmio->data);
    return MMSYSERR_NOERROR;
}

/* Nothing follows the one buffer: leave pchNext at pchEndRead (end of
 * file), which WaveReadFile reports as a corrupt file. */
MMRESULT mmioAdvance(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    (void)flags;
    if (hmmio == NULL || info == NULL) return MMSYSERR_INVALHANDLE;
    hmmio->pos = (LONG)(info->pchNext - hmmio->data);
    return mmioGetInfo(hmmio, info, 0);
}

static DWORD winmm_port_le32(const char* p)
{
    const unsigned char* b = (const unsigned char*)p;
    return (DWORD)b[0] | ((DWORD)b[1] << 8) | ((DWORD)b[2] << 16) |
           ((DWORD)b[3] << 24);
}

MMRESULT mmioDescend(HMMIO hmmio, LPMMCKINFO chunk, const MMCKINFO* parent,
                     UINT flags)
{
    int64_t end;
    int64_t pos;
    if (hmmio == NULL || chunk == NULL) return MMSYSERR_INVALHANDLE;
    end = hmmio->length;
    if (parent != NULL) {
        int64_t parent_end = (int64_t)parent->dwDataOffset + parent->cksize;
        if (parent_end < end) end = parent_end;
    }
    pos = hmmio->pos;
    for (;;) {
        FOURCC ckid;
        DWORD cksize;
        FOURCC fccType = 0;
        int64_t data_offset;
        int is_form;
        int match;
        if (pos + 8 > end) return MMIOERR_CHUNKNOTFOUND;
        ckid = winmm_port_le32(hmmio->data + pos);
        cksize = winmm_port_le32(hmmio->data + pos + 4);
        data_offset = pos + 8;
        is_form = ckid == FOURCC_RIFF || ckid == FOURCC_LIST;
        if (is_form) {
            if (data_offset + 4 > hmmio->length) return MMIOERR_CHUNKNOTFOUND;
            fccType = winmm_port_le32(hmmio->data + data_offset);
        }
        if (flags & MMIO_FINDCHUNK)
            match = ckid == chunk->ckid;
        else if (flags & MMIO_FINDRIFF)
            match = ckid == FOURCC_RIFF && fccType == chunk->fccType;
        else if (flags & MMIO_FINDLIST)
            match = ckid == FOURCC_LIST && fccType == chunk->fccType;
        else
            match = 1;
        if (match) {
            chunk->ckid = ckid;
            chunk->cksize = cksize;
            chunk->fccType = fccType;
            chunk->dwDataOffset = (DWORD)data_offset;
            chunk->dwFlags = 0;
            hmmio->pos = (LONG)(data_offset + (is_form ? 4 : 0));
            return MMSYSERR_NOERROR;
        }
        pos = data_offset + cksize + (cksize & 1u);
    }
}

MMRESULT mmioAscend(HMMIO hmmio, LPMMCKINFO chunk, UINT flags)
{
    int64_t end;
    (void)flags;
    if (hmmio == NULL || chunk == NULL) return MMSYSERR_INVALHANDLE;
    end = (int64_t)chunk->dwDataOffset + chunk->cksize + (chunk->cksize & 1u);
    if (end > hmmio->length) return MMIOERR_CANNOTSEEK;
    hmmio->pos = (LONG)end;
    return MMSYSERR_NOERROR;
}

MCIERROR mciSendStringA(LPCSTR command, LPSTR result, UINT capacity,
                        HWND callback)
{
    (void)callback;
    if (result != NULL && capacity > 0) result[0] = '\0';
    if (s_mci_hook != NULL) return s_mci_hook(command, result, capacity);
    return MCIERR_INVALID_DEVICE_NAME;
}

BOOL mciGetErrorStringA(MCIERROR error, LPSTR text, UINT capacity)
{
    if (text == NULL || capacity == 0) return FALSE;
    if (error == MCIERR_INVALID_DEVICE_NAME)
        snprintf(text, capacity,
                 "The specified device is not open or is not recognized by MCI.");
    else
        snprintf(text, capacity, "MCI error %u.", (unsigned)error);
    return TRUE;
}

} /* extern "C" */

#endif /* !_WIN32 */
