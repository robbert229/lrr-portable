// SPDX-License-Identifier: LGPL-3.0-or-later
/* The seam's imports that neither miniwin nor a library substitution
 * provides yet (ADR 0007): the Win32 and DirectX calls outside miniwin's
 * scope that the units make. Each stub logs its name once and returns the
 * failure its API documents, which the units already handle; none aborts.
 * The library substitutions of ADR 0007 replace these calls in the units:
 *
 *   DSOUND, WINMM mmio/MCI/mixer  -> [library:audio]
 *   AVIFIL32, OLE32 (DirectShow)  -> [library:video]
 *   ADVAPI32 registry             -> [library:config]
 *   LIBC directory routines,
 *   KERNEL32 GetDriveTypeA        -> [library:filesystem]
 *
 * A few calls are small enough to work here already (GlobalAlloc,
 * GlobalFree, wsprintfA, GetLastError, MultiByteToWideChar, the working
 * directory routines, OutputDebugStringA and GetCommandLineA, whose string
 * the process entry builds); they are marked as working. */
#include <SDL3/SDL.h>

#include <miniwin/windows.h>

#include "gods98/files_native.h"
#include "gods98/registry_api.h"
#include "gods98/winmm.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

namespace {

/* Logs the stub's name the first time it is called. */
void SeamStub(const char* name)
{
    static std::set<std::string> logged;
    if (logged.insert(name).second) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "seam stub: %s", name);
    }
}

#define SEAM_STUB() SeamStub(__func__)

DWORD g_lastError = 0;

} // namespace

extern "C" {

/* --- KERNEL32 --------------------------------------------------------- */

/* The process's command line, which entry.cpp builds from argv. */
char seam_default_command_line[] = "\"LegoRR.exe\"";
char* seam_command_line = seam_default_command_line;

/* Working. */
LPSTR GetCommandLineA(void)
{
    return seam_command_line;
}

/* Stub: the single-instance mutex. No other instance is ever found. */
HANDLE CreateMutexA(void* attributes, BOOL initialOwner, LPCSTR name)
{
    SEAM_STUB();
    g_lastError = 0;
    static int mutex;
    return &mutex;
}

/* Working. */
DWORD GetLastError(void)
{
    return g_lastError;
}

/* Working: GMEM_FIXED memory is plain heap memory. */
HGLOBAL GlobalAlloc(UINT flags, size_t bytes)
{
    return calloc(1, bytes ? bytes : 1);
}

/* Working. */
HGLOBAL GlobalFree(HGLOBAL memory)
{
    free(memory);
    return NULL;
}

/* Working: USER32's wsprintfA writes at most 1024 characters. */
int wsprintfA(LPSTR output, LPCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf(output, 1025, format, args);
    va_end(args);
    return written;
}

/* Working: the units only pass ASCII paths. */
int MultiByteToWideChar(UINT codePage, DWORD flags, const char* text,
                        int textLength, uint16_t* wide, int wideLength)
{
    int length = textLength < 0 ? (int)strlen(text) + 1 : textLength;
    if (!wide || wideLength == 0) return length;
    if (length > wideLength) length = wideLength;
    for (int i = 0; i < length; i++) wide[i] = (unsigned char)text[i];
    return length;
}

/* --- WINMM: mmio ([library:audio]) ------------------------------------- */

/* Stub: no file opens, so no sample loads. */
HMMIO mmioOpenA(LPSTR name, LPMMIOINFO info, DWORD flags)
{
    SEAM_STUB();
    if (info) info->wErrorRet = MMIOERR_CANNOTOPEN;
    return NULL;
}

MMRESULT mmioClose(HMMIO hmmio, UINT flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

LONG mmioRead(HMMIO hmmio, HPSTR buffer, LONG bytes)
{
    SEAM_STUB();
    return -1;
}

LONG mmioSeek(HMMIO hmmio, LONG offset, int origin)
{
    SEAM_STUB();
    return -1;
}

MMRESULT mmioGetInfo(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mmioSetInfo(HMMIO hmmio, const MMIOINFO* info, UINT flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mmioAdvance(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    SEAM_STUB();
    return MMIOERR_CANNOTREAD;
}

MMRESULT mmioDescend(HMMIO hmmio, LPMMCKINFO chunk, const MMCKINFO* parent,
                     UINT flags)
{
    SEAM_STUB();
    return MMIOERR_CHUNKNOTFOUND;
}

MMRESULT mmioAscend(HMMIO hmmio, LPMMCKINFO chunk, UINT flags)
{
    SEAM_STUB();
    return MMIOERR_CANNOTSEEK;
}

/* --- WINMM: MCI and the mixer ([library:audio]) ------------------------ */

/* Stub: there is no "cdaudio" device. */
MCIERROR mciSendStringA(LPCSTR command, LPSTR result, UINT capacity,
                        HWND callback)
{
    SEAM_STUB();
    if (result && capacity) result[0] = '\0';
    return MCIERR_INVALID_DEVICE_NAME;
}

BOOL mciGetErrorStringA(MCIERROR error, LPSTR text, UINT capacity)
{
    SEAM_STUB();
    if (!text || !capacity) return FALSE;
    snprintf(text, capacity, "MCI error %u", (unsigned)error);
    return TRUE;
}

/* Stub: no mixer devices. */
UINT mixerGetNumDevs(void)
{
    SEAM_STUB();
    return 0;
}

MMRESULT mixerOpen(HMIXER* mixer, UINT device, DWORD_PTR callback,
                   DWORD_PTR instance, DWORD flags)
{
    SEAM_STUB();
    if (mixer) *mixer = NULL;
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerClose(HMIXER mixer)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerGetDevCapsA(UINT_PTR device, MIXERCAPSA* caps, UINT size)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerGetLineInfoA(HMIXEROBJ mixer, MIXERLINEA* line, DWORD flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerGetLineControlsA(HMIXEROBJ mixer, MIXERLINECONTROLSA* controls,
                               DWORD flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerGetControlDetailsA(HMIXEROBJ mixer,
                                 MIXERCONTROLDETAILS* details, DWORD flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

MMRESULT mixerSetControlDetails(HMIXEROBJ mixer,
                                MIXERCONTROLDETAILS* details, DWORD flags)
{
    SEAM_STUB();
    return MMSYSERR_INVALHANDLE;
}

/* --- DSOUND ([library:audio]) ------------------------------------------ */

/* Stub: no DirectSound device, so Sound3D_Initialise runs without sound.
 * sound3d.c declares the parameters with its own interface types. */
long DirectSoundCreate(void* guid, void** device, void* outer)
{
    SEAM_STUB();
    if (device) *device = NULL;
    return (long)0x88780078; /* DSERR_NODRIVER */
}

/* --- ADVAPI32 ([library:config]) --------------------------------------- */

/* Stub: an empty registry, so every value takes its default. */
LONG RegCreateKeyA(HKEY key, const char* subkey, HKEY* result)
{
    SEAM_STUB();
    if (result) *result = 0;
    return 2; /* ERROR_FILE_NOT_FOUND */
}

LONG RegCloseKey(HKEY key)
{
    SEAM_STUB();
    return 0;
}

LONG RegQueryValueExA(HKEY key, const char* name, DWORD* reserved,
                      DWORD* type, unsigned char* data, DWORD* size)
{
    SEAM_STUB();
    return 2; /* ERROR_FILE_NOT_FOUND */
}

/* --- AVIFIL32 ([library:video]) ---------------------------------------- */

/* Stub: no AVI opens. avi_player.cpp declares the parameters with its own
 * types. */
void AVIFileInit(void)
{
    SEAM_STUB();
}

HRESULT AVIStreamOpenFromFileA(void** ppavi, LPCSTR file, DWORD type,
                               LONG param, UINT mode, void* handler)
{
    SEAM_STUB();
    if (ppavi) *ppavi = NULL;
    return (HRESULT)0x80044067; /* AVIERR_FILEOPEN */
}

HRESULT AVIStreamInfoA(void* pavi, void* info, LONG size)
{
    SEAM_STUB();
    return (HRESULT)0x80044067;
}

ULONG AVIStreamRelease(void* pavi)
{
    SEAM_STUB();
    return 0;
}

void* AVIStreamGetFrameOpen(void* pavi, LPBITMAPINFOHEADER wanted)
{
    SEAM_STUB();
    return NULL;
}

LPVOID AVIStreamGetFrame(void* frame, LONG position)
{
    SEAM_STUB();
    return NULL;
}

/* --- OLE32: DirectShow's multimedia streams ([library:video]) ----------- */

/* Stub: COM starts, but no class is registered. */
HRESULT CoInitialize(void* reserved)
{
    SEAM_STUB();
    return S_OK;
}

HRESULT CoCreateInstance(const GUID& clsid, void* outer, DWORD context,
                         const GUID& iid, void** object)
{
    SEAM_STUB();
    if (object) *object = NULL;
    return (HRESULT)0x80040154; /* REGDB_E_CLASSNOTREG */
}

/* --- LIBC directory routines and KERNEL32 ([library:filesystem]) ------- */

/* Stub: no directory listing. */
long files_native_findfirst(const char* filespec,
                            struct files_native_finddata* fileinfo)
{
    SEAM_STUB();
    return -1;
}

int files_native_findnext(long handle, struct files_native_finddata* fileinfo)
{
    SEAM_STUB();
    return -1;
}

int files_native_findclose(long handle)
{
    SEAM_STUB();
    return -1;
}

/* Stub: attributes are not changed. */
int files_native_chmod(const char* path, int mode)
{
    SEAM_STUB();
    return 0;
}

/* Working. */
char* files_native_getcwd(char* buffer, int maxlen)
{
    char* cwd = SDL_GetCurrentDirectory();
    if (!cwd) return NULL;
    size_t length = strlen(cwd);
    /* SDL ends the directory with a separator; _getcwd does not. */
    if (length > 1 && (cwd[length - 1] == '/' || cwd[length - 1] == '\\')) cwd[--length] = '\0';
    if (length + 1 > (size_t)maxlen) {
        SDL_free(cwd);
        return NULL;
    }
    memcpy(buffer, cwd, length + 1);
    SDL_free(cwd);
    return buffer;
}

/* Working. */
int files_native_mkdir(const char* path)
{
    return SDL_CreateDirectory(path) ? 0 : -1;
}

/* [library:filesystem] Working. The units build Windows paths: '\\'
 * separators, and names whose case need not match the files on disk. */
namespace {

struct CaseMatch {
    const char* want;
    std::string found;
};

SDL_EnumerationResult SDLCALL MatchEntryCase(void* userdata, const char* dirname,
                                             const char* fname)
{
    (void)dirname;
    CaseMatch* match = static_cast<CaseMatch*>(userdata);
    if (SDL_strcasecmp(fname, match->want) == 0) {
        match->found = fname;
        return SDL_ENUM_SUCCESS;
    }
    return SDL_ENUM_CONTINUE;
}

/* The host path for a Windows one: each component that does not exist as
 * spelled takes the spelling of an entry that matches it without case. */
std::string HostPath(const char* path)
{
    std::string text(path);
    for (char& c : text) {
        if (c == '\\') c = '/';
    }
    std::string resolved;
    size_t start = 0;
    if (!text.empty() && text[0] == '/') {
        resolved = "/";
        start = 1;
    }
    while (start <= text.size()) {
        size_t end = text.find('/', start);
        if (end == std::string::npos) end = text.size();
        std::string part = text.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") continue;
        std::string dir = resolved.empty() ? "." : resolved;
        std::string candidate = resolved.empty() ? part :
            (resolved == "/" ? "/" + part : resolved + "/" + part);
        SDL_PathInfo info;
        if (part != ".." && !SDL_GetPathInfo(candidate.c_str(), &info)) {
            CaseMatch match = { part.c_str(), std::string() };
            SDL_EnumerateDirectory(dir.c_str(), MatchEntryCase, &match);
            if (!match.found.empty()) {
                candidate = resolved.empty() ? match.found :
                    (resolved == "/" ? "/" + match.found : resolved + "/" + match.found);
            }
        }
        resolved = candidate;
    }
    return resolved;
}

} // namespace

FILE* files_native_fopen(const char* path, const char* mode)
{
    return fopen(HostPath(path).c_str(), mode);
}

/* Working: a relative path is taken from the working directory. */
char* files_native_fullpath(char* absPath, const char* relPath,
                            unsigned int maxLength)
{
    char cwd[1024];
    if (relPath[0] == '/' || relPath[0] == '\\' ||
        (relPath[0] != '\0' && relPath[1] == ':')) {
        if (strlen(relPath) + 1 > maxLength) return NULL;
        strcpy(absPath, relPath);
        return absPath;
    }
    if (!files_native_getcwd(cwd, sizeof(cwd))) return NULL;
    if ((unsigned)snprintf(absPath, maxLength, "%s/%s", cwd, relPath) >= maxLength) return NULL;
    return absPath;
}

/* Stub: no drive is a CD-ROM drive (DRIVE_UNKNOWN). */
unsigned int files_native_get_drive_type(const char* rootPathName)
{
    SEAM_STUB();
    return 0;
}

/* Working. */
void files_native_output_debug_string(const char* text)
{
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", text);
}

} /* extern "C" */
