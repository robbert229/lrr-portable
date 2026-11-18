// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-001 (text 004721e0-00472710; findings 505
 * and 618): gods98's movie player, which plays a file through DirectShow's
 * multimedia streaming (amstream: CLSID_AMMultiMediaStream). It is C++ in
 * the original (finding 505 rule 8): one class with thiscall methods and an
 * EH frame in movie_load, and five extern "C" entry points.
 *
 * G98CMovie (0x38 bytes) opens the file into an IAMMultiMediaStream with a
 * DirectDraw video stream and the default audio renderer, creates one
 * stream sample, and blits the sample's surface to the back buffer (or a
 * rectangle of it) on each Update.
 *
 * The binary attests none of the names (it has no RTTI). The class is named
 * after gods98's own movie class; the methods use descriptive names.
 *
 * DirectShow is outside miniwin's scope. ADR 0007's video substitution
 * replaces it with FFmpeg (library/video_ffmpeg.h), call by call, at the
 * sites marked `// [library:video]`: the multimedia stream becomes a
 * VideoFile, which also plays the audio track, and the stream sample
 * becomes a miniwin surface of the movie's size that each Update decodes
 * the next frame into before the blit. Without FFmpeg no stream opens,
 * Update reports no frame, and the callers skip the movie. */
#include "movie_unit.h"


#include <stdio.h>
#include <string.h>
#include <miniwin/windows.h>
#include <miniwin/ddraw.h>

#include "files.h"
#include "files_native.h"
#include "library/video_ffmpeg.h"
#include "../game/aitask.h"
#include "../crt/vc5names.h"

/* The original links seven GUIDs from amstrmid.lib and dxguid.lib
 * (0049f168-0049f358): the multimedia stream's class and interfaces, the
 * primary video and audio stream ids, IDirectDraw2 and
 * IDirectDrawSurface3. The video substitution needs none of them; it asks
 * miniwin for the DirectDraw 4 interfaces the units use elsewhere. */

typedef int64_t LONGLONG;

#define MOVIE_MAX_PATH                  1024
#define MOVIE_E_FAIL                    ((HRESULT)0x80004005)
#define MOVIE_DDBLT_WAIT                0x01000000
#define MOVIE_DD_OK                     0

/* Callees declared by their own headers: aitask.h gives the DirectDraw
 * object (renderer_global_dword_get, 0x004065d0) and the back buffer
 * (render_surface_secondary_get, 0x004065e0), AITask.c's out-of-line copies
 * of the DirectDraw() and DirectDraw_bSurf() accessors; files.h gives
 * File_VerifyFilename and File_GetCDFilePath. */

#define MOVIE_OPERATOR_NEW(size) ::operator new(size)

struct G98CMovie {
    G98CMovie(const char* fName, IDirectDrawSurface4* destSurf,
              IDirectDraw4* ddraw);
    ~G98CMovie();
    bool InitSample(VideoFile* stream);
    bool OpenStream(const char* fName, VideoFile** streamOut,
                    IDirectDraw4* ddraw);
    bool Update(float speed, LPRECT destRect);
    unsigned int GetDuration();

    /* Deviation (scoring only): the original's new-expression and
     * destructor call the global operator new (LIBC's, 0x0048ead0) and
     * delete (0x0045daf0), as in avi_player.cpp. */
    // PORT-ONLY: routes new to the original's operator new (0x0048ead0)
    static void* operator new(size_t size) { return MOVIE_OPERATOR_NEW(size); }
    // PORT-ONLY: routes delete to the original's operator delete (0x0045daf0)
    static void operator delete(void* p) { ::operator delete(p); }

    /* The original's IMediaStream (+0x08), IDirectDrawMediaStream (+0x0c),
     * the sample's IDirectDrawSurface (+0x10) and the stream sample (+0x18)
     * have no counterpart in the video substitution: the VideoFile is the
     * stream, and m_baseSurf the sample. */
    VideoFile* m_amStream;              /* +0x00 IAMMultiMediaStream */
    HRESULT m_err;                      /* +0x04 */
    IDirectDrawSurface4* m_baseSurf;    /* +0x14 the sample's surface */
    IDirectDrawSurface4* m_destSurf;    /* +0x1c the back buffer */
    RECT m_rect;                        /* +0x20 */
    char* m_filename;                   /* +0x30 */
    IDirectDraw4* m_ddraw;              /* +0x34 IDirectDraw2 */
};

// FUNCTION: LEGORR 0x004721e0
bool G98CMovie::InitSample(VideoFile* stream)
{
    DDSURFACEDESC2 desc;
    unsigned int width, height;

    /* The sample: DirectShow's CreateSample makes a surface in the video's
     * size and format, and GetSurface returns it with its rectangle. */
    video_get_size(stream, &width, &height); // [library:video]
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    m_err = m_ddraw->CreateSurface(&desc, &m_baseSurf, NULL); // [library:video]
    if ((m_err) != S_OK) goto fail;
    m_rect.left = 0; // [library:video]
    m_rect.top = 0;
    m_rect.right = (LONG)width;
    m_rect.bottom = (LONG)height;
    video_start(stream); // [library:video]

    return true;

fail:
    m_baseSurf = NULL;

    return (m_err) == S_OK ? true : false;
}

// FUNCTION: LEGORR 0x004722b0
bool G98CMovie::OpenStream(const char* fName, VideoFile** streamOut,
                           IDirectDraw4* ddraw)
{
    VideoFile* stream = NULL;
    char hostPath[MOVIE_MAX_PATH];
    HRESULT hr;

    /* CoCreateInstance, Initialize, the video and default-audio
     * AddMediaStream calls and OpenFile: one open of the file, with its
     * audio track. */
    if (files_native_host_path(hostPath, fName, sizeof(hostPath))) { // [library:filesystem]
        stream = video_open(hostPath, TRUE); // [library:video]
    }
    hr = stream ? S_OK : MOVIE_E_FAIL;
    if ((hr) == S_OK) {
        *streamOut = stream;
    }

    return (hr) == S_OK ? true : false;
}

// FUNCTION: LEGORR 0x00472390
G98CMovie::G98CMovie(const char* fName, IDirectDrawSurface4* destSurf,
                     IDirectDraw4* ddraw)
{
    /* The original leaves m_amStream and m_filename unset; they are read
     * when no stream opens. */
    m_amStream = NULL;
    m_filename = NULL;
    m_baseSurf = NULL;
    m_destSurf = destSurf;
    m_ddraw = ddraw;

    /* The original calls CoInitialize here; FFmpeg needs no COM. */

    if (fName && strlen(fName)) {
        m_filename = (char*)malloc(strlen(fName) + 1);
        strcpy(m_filename, fName);
        if (OpenStream(fName, &m_amStream, ddraw)) InitSample(m_amStream);
    }
}

// FUNCTION: LEGORR 0x00472430
G98CMovie::~G98CMovie()
{
    if (m_baseSurf) m_baseSurf->Release();
    if (m_destSurf) m_destSurf->Release();
    if (m_ddraw) m_ddraw->Release();
    if (m_amStream) video_close(m_amStream); // [library:video]
    if (m_filename) free(m_filename);
}

// FUNCTION: LEGORR 0x004724a0
bool G98CMovie::Update(float speed, LPRECT destRect)
{
    DDSURFACEDESC2 desc;
    int frame = 0;

    if (m_baseSurf) {
        /* The sample's synchronous Update: wait for the next frame and
         * decode it into the sample's surface. */
        memset(&desc, 0, sizeof(desc)); // [library:video]
        desc.dwSize = sizeof(desc);
        if (m_baseSurf->Lock(NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK) {
            frame = video_update(m_amStream, desc.lpSurface, desc.lPitch,
                                 desc.ddpfPixelFormat.dwRGBBitCount,
                                 desc.ddpfPixelFormat.dwRBitMask,
                                 desc.ddpfPixelFormat.dwGBitMask,
                                 desc.ddpfPixelFormat.dwBBitMask);
            m_baseSurf->Unlock(NULL);
        }
        if (!frame) {
            m_baseSurf->Release();
            m_baseSurf = NULL;
            if (m_amStream) {
                video_close(m_amStream); // [library:video]
                m_amStream = NULL;
            }
            return false;
        }
        return (m_destSurf->Blt(destRect, m_baseSurf, NULL,
                                (DDBltFlags)MOVIE_DDBLT_WAIT,
                                NULL) == MOVIE_DD_OK) ? true : false;
    }

    return false;
}

// FUNCTION: LEGORR 0x00472530
unsigned int G98CMovie::GetDuration()
{
    LONGLONG duration;

    /* GetDuration's 100 ns units, from the video's milliseconds. */
    duration = m_amStream ? (LONGLONG)video_get_duration(m_amStream) * 10000 // [library:video]
                          : 0;

    return (unsigned int)(duration / 10000);
}

extern "C" {

// FUNCTION: LEGORR 0x00472560
G98CMovie* movie_load(const char* fName)
{
    const char* fullName = file_verify_filename(fName);
    G98CMovie* mov = NULL;
    IDirectDraw4* ddraw = NULL;
    IDirectDrawSurface4* surf = NULL;
    char cdName[512];
    FILE* fp;

    /* The original asks for the IDirectDraw2 and IDirectDrawSurface3 that
     * DirectShow takes; the video substitution draws through the DirectDraw
     * 4 interfaces. */
    renderer_global_dword_get()->QueryInterface(IID_IDirectDraw4, (void**)&ddraw); // [library:video]
    render_surface_secondary_get()->QueryInterface(IID_IDirectDrawSurface4, (void**)&surf); // [library:video]

    if ((fp = files_native_fopen(fullName, "r")) != NULL) { // [library:filesystem]
        fclose(fp);
        mov = new G98CMovie(fullName, surf, ddraw);
    } else if (file_get_cd_file_path(cdName, fName)) {
        mov = new G98CMovie(cdName, surf, ddraw);
    }

    return mov;
}

// FUNCTION: LEGORR 0x00472670
void movie_get_size(G98CMovie* mov, unsigned int* width,
                    unsigned int* height)
{
    DDSURFACEDESC2 desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    /* The original reads a movie that did not open through a NULL
     * surface. */
    if (mov->m_baseSurf) mov->m_baseSurf->GetSurfaceDesc(&desc);
    *width = desc.dwWidth;
    *height = desc.dwHeight;
}

// FUNCTION: LEGORR 0x004726c0
unsigned int movie_get_duration(G98CMovie* mov)
{
    return mov->GetDuration();
}

// FUNCTION: LEGORR 0x004726d0
int movie_update(G98CMovie* mov, float speed, LPRECT destRect)
{
    return mov->Update(speed, destRect) ? 1 : 0;
}

// FUNCTION: LEGORR 0x004726f0
void movie_free(G98CMovie* mov)
{
    delete mov;
}

} /* extern "C" */
