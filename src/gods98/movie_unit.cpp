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
 * DirectShow is outside miniwin's scope: until the video substitution of
 * ADR 0007 ([library:video], FFmpeg) replaces it, CoInitialize and
 * CoCreateInstance are seam stubs (platform/native/seam_stubs.cpp), so no
 * movie opens. */
#include "movie_unit.h"


#include <stdio.h>
#include <string.h>
#include <miniwin/windows.h>

#include "files.h"
#include "files_native.h"
#include "../game/aitask.h"
#include "../crt/vc5names.h"

/* The GUIDs, which the original links from amstrmid.lib (the first five)
 * and dxguid.lib (the last two). */
// GLOBAL: LEGORR 0x0049f168
static const GUID IID_IAMMultiMediaStream =
    {0xbebe595c, 0x9a6f, 0x11d0, {0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d}};
// GLOBAL: LEGORR 0x0049f1e8
static const GUID CLSID_AMMultiMediaStream =
    {0x49c47ce5, 0x9ba4, 0x11d0, {0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}};
// GLOBAL: LEGORR 0x0049f228
static const GUID IID_IDirectDrawMediaStream =
    {0xf4104fce, 0x9a70, 0x11d0, {0x8f, 0xde, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d}};
// GLOBAL: LEGORR 0x0049f288
static const GUID MSPID_PrimaryVideo =
    {0xa35ff56a, 0x9fda, 0x11d0, {0x8f, 0xdf, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d}};
// GLOBAL: LEGORR 0x0049f298
static const GUID MSPID_PrimaryAudio =
    {0xa35ff56b, 0x9fda, 0x11d0, {0x8f, 0xdf, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d}};
// GLOBAL: LEGORR 0x0049f318
static const GUID IID_IDirectDraw2 =
    {0xb3a6f3e0, 0x2b43, 0x11cf, {0xa2, 0xde, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56}};
// GLOBAL: LEGORR 0x0049f358
static const GUID IID_IDirectDrawSurface3 =
    {0xda044e00, 0x69b2, 0x11d0, {0xa1, 0xd5, 0x00, 0xaa, 0x00, 0xb8, 0xdf, 0xbb}};

/* OLE32 and KERNEL32, outside miniwin's scope. platform/native/
 * seam_stubs.cpp stubs them until the video substitution of ADR 0007
 * replaces DirectShow in this unit. */
typedef int64_t LONGLONG;
typedef uint16_t WCHAR;
typedef const WCHAR* LPCWSTR;
#define MAX_PATH 260
#define CP_ACP 0
#define CLSCTX_INPROC_SERVER 0x1
extern "C" {
HRESULT CoInitialize(void* reserved);
HRESULT CoCreateInstance(const GUID& clsid, void* outer, DWORD context,
                         const GUID& iid, void** object);
int MultiByteToWideChar(UINT codePage, DWORD flags, const char* text,
                        int textLength, WCHAR* wide, int wideLength);
}

/* MSVC 5.0 has no amstream.h, and its ddraw.h is DirectX 3's. The unit
 * declares the interfaces it calls, each up to the last slot it uses, as
 * C++ abstract structs with virtuals, so the calls compile to the
 * original's `mov ecx, [eax]` ... `call [ecx+slot]` shape. */

/* DDSURFACEDESC (0x6c bytes). */
struct MovieSurfaceDesc {
    DWORD dwSize;                       /* +0x00 */
    DWORD dwFlags;                      /* +0x04 */
    DWORD dwHeight;                     /* +0x08 */
    DWORD dwWidth;                      /* +0x0c */
    DWORD rest[23];                     /* +0x10 */
};

/* IUnknown: IMediaStream, IDirectDrawSurface and IDirectDraw2, which the
 * unit only queries and releases. */
struct MovieUnknown {
    virtual HRESULT QueryInterface(const GUID&, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
};

/* IDirectDrawSurface3, up to GetSurfaceDesc (slot 22). */
struct MovieSurface3 {
    virtual HRESULT QueryInterface(const GUID&, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    virtual HRESULT AddAttachedSurface(void*) = 0;
    virtual HRESULT AddOverlayDirtyRect(LPRECT) = 0;
    virtual HRESULT Blt(LPRECT, MovieSurface3*, LPRECT, DWORD,
                                  void*) = 0;
    virtual HRESULT BltBatch(void*, DWORD, DWORD) = 0;
    virtual HRESULT BltFast(DWORD, DWORD, MovieSurface3*, LPRECT,
                                      DWORD) = 0;
    virtual HRESULT DeleteAttachedSurface(DWORD, void*) = 0;
    virtual HRESULT EnumAttachedSurfaces(void*, void*) = 0;
    virtual HRESULT EnumOverlayZOrders(DWORD, void*, void*) = 0;
    virtual HRESULT Flip(void*, DWORD) = 0;
    virtual HRESULT GetAttachedSurface(void*, void**) = 0;
    virtual HRESULT GetBltStatus(DWORD) = 0;
    virtual HRESULT GetCaps(void*) = 0;
    virtual HRESULT GetClipper(void**) = 0;
    virtual HRESULT GetColorKey(DWORD, void*) = 0;
    virtual HRESULT GetDC(HDC*) = 0;
    virtual HRESULT GetFlipStatus(DWORD) = 0;
    virtual HRESULT GetOverlayPosition(LPLONG, LPLONG) = 0;
    virtual HRESULT GetPalette(void**) = 0;
    virtual HRESULT GetPixelFormat(void*) = 0;
    virtual HRESULT GetSurfaceDesc(MovieSurfaceDesc*) = 0;
};

/* IDirectDrawStreamSample, up to GetSurface (slot 8). */
struct MovieStreamSample {
    virtual HRESULT QueryInterface(const GUID&, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    virtual HRESULT GetMediaStream(void**) = 0;
    virtual HRESULT GetSampleTimes(LONGLONG*, LONGLONG*,
                                             LONGLONG*) = 0;
    virtual HRESULT SetSampleTimes(const LONGLONG*,
                                             const LONGLONG*) = 0;
    virtual HRESULT Update(DWORD, HANDLE, void*, DWORD) = 0;
    virtual HRESULT CompletionStatus(DWORD, DWORD) = 0;
    virtual HRESULT GetSurface(MovieUnknown**, RECT*) = 0;
};

/* IDirectDrawMediaStream, up to CreateSample (slot 13). */
struct MovieDDStream {
    virtual HRESULT QueryInterface(const GUID&, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    virtual HRESULT GetMultiMediaStream(void**) = 0;
    virtual HRESULT GetInformation(void*, void*) = 0;
    virtual HRESULT SetSameFormat(void*, DWORD) = 0;
    virtual HRESULT AllocateSample(DWORD, void**) = 0;
    virtual HRESULT CreateSharedSample(void*, DWORD, void**) = 0;
    virtual HRESULT SendEndOfStream(DWORD) = 0;
    virtual HRESULT GetFormat(void*, void**, void*, DWORD*) = 0;
    virtual HRESULT SetFormat(const void*, void*) = 0;
    virtual HRESULT GetDirectDraw(void**) = 0;
    virtual HRESULT SetDirectDraw(void*) = 0;
    virtual HRESULT CreateSample(void*, const RECT*, DWORD,
                                           MovieStreamSample**) = 0;
};

/* IAMMultiMediaStream, up to OpenFile (slot 16). */
struct MovieAMStream {
    virtual HRESULT QueryInterface(const GUID&, void**) = 0;
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    virtual HRESULT GetInformation(DWORD*, DWORD*) = 0;
    virtual HRESULT GetMediaStream(const GUID&,
                                             MovieUnknown**) = 0;
    virtual HRESULT EnumMediaStreams(long, void**) = 0;
    virtual HRESULT GetState(DWORD*) = 0;
    virtual HRESULT SetState(DWORD) = 0;
    virtual HRESULT GetTime(LONGLONG*) = 0;
    virtual HRESULT GetDuration(LONGLONG*) = 0;
    virtual HRESULT Seek(LONGLONG) = 0;
    virtual HRESULT GetEndOfStreamEventHandle(HANDLE*) = 0;
    virtual HRESULT Initialize(DWORD, DWORD, void*) = 0;
    virtual HRESULT GetFilterGraph(void**) = 0;
    virtual HRESULT GetFilter(void**) = 0;
    virtual HRESULT AddMediaStream(void*, const GUID*, DWORD,
                                             void**) = 0;
    virtual HRESULT OpenFile(LPCWSTR, DWORD) = 0;
};

/* The methods' `cond ? true : false` returns, and InitSample's goto chain,
 * are the shapes that give the original's code: `return cond;` sets the
 * bool through `xor eax, eax` plus `setcc` (or `neg`/`sbb`/`inc`) instead
 * of a bare `setcc`. */

#define MOVIE_STREAMTYPE_READ           0
#define MOVIE_STREAMSTATE_RUN           1
#define MOVIE_AMMSF_ADDDEFAULTRENDERER  0x00000001
#define MOVIE_DDBLT_WAIT                0x01000000
#define MOVIE_DD_OK                     0

/* Callees declared by their own headers: aitask.h gives the DirectDraw
 * object (renderer_global_dword_get, 0x004065d0) and the back buffer
 * (render_surface_secondary_get, 0x004065e0), AITask.c's out-of-line copies
 * of the DirectDraw() and DirectDraw_bSurf() accessors; heap.hpp gives the
 * native port's operator new (engine_calloc; the matching build calls
 * LIBC's, 0x0048ead0) and operator delete (ui_render_record_free,
 * 0x0045daf0); files.h gives File_VerifyFilename and File_GetCDFilePath. */

#define MOVIE_OPERATOR_NEW(size) ::operator new(size)

struct G98CMovie {
    G98CMovie(const char* fName, MovieSurface3* destSurf,
              MovieUnknown* ddraw);
    ~G98CMovie();
    bool InitSample(MovieAMStream* stream);
    bool OpenStream(const char* fName, MovieAMStream** streamOut,
                    MovieUnknown* ddraw);
    bool Update(float speed, LPRECT destRect);
    unsigned int GetDuration();
    void SetPosition(unsigned int ms);

    /* Deviation (scoring only): the original's new-expression and
     * destructor call the global operator new (LIBC's, 0x0048ead0) and
     * delete (0x0045daf0), as in avi_player.cpp. */
    // PORT-ONLY: routes new to the original's operator new (0x0048ead0)
    static void* operator new(size_t size) { return MOVIE_OPERATOR_NEW(size); }
    // PORT-ONLY: routes delete to the original's operator delete (0x0045daf0)
    static void operator delete(void* p) { ::operator delete(p); }

    MovieAMStream* m_amStream;          /* +0x00 */
    HRESULT m_err;                      /* +0x04 */
    MovieUnknown* m_primaryVidStream;   /* +0x08 IMediaStream */
    MovieDDStream* m_ddStream;          /* +0x0c */
    MovieUnknown* m_surface;            /* +0x10 IDirectDrawSurface */
    MovieSurface3* m_baseSurf;          /* +0x14 */
    MovieStreamSample* m_sample;        /* +0x18 */
    MovieSurface3* m_destSurf;          /* +0x1c the back buffer */
    RECT m_rect;                        /* +0x20 */
    char* m_filename;                   /* +0x30 */
    MovieUnknown* m_ddraw;              /* +0x34 IDirectDraw2 */
};

// FUNCTION: LEGORR 0x004721e0
bool G98CMovie::InitSample(MovieAMStream* stream)
{
    m_err = stream->GetMediaStream(MSPID_PrimaryVideo, &m_primaryVidStream);
    if ((m_err) != S_OK) goto fail;
    m_err = m_primaryVidStream->QueryInterface(IID_IDirectDrawMediaStream,
                                               (void**)&m_ddStream);
    if ((m_err) != S_OK) goto fail;
    m_err = m_ddStream->CreateSample(NULL, NULL, 0, &m_sample);
    if ((m_err) != S_OK) goto fail;
    m_err = m_sample->GetSurface(&m_surface, &m_rect);
    if ((m_err) != S_OK) goto fail;
    m_err = m_surface->QueryInterface(IID_IDirectDrawSurface3,
                                      (void**)&m_baseSurf);
    if ((m_err) != S_OK) goto fail;
    m_err = stream->SetState(MOVIE_STREAMSTATE_RUN);
    if ((m_err) != S_OK) goto fail;

    return true;

fail:
    m_primaryVidStream->Release();
    m_ddStream->Release();
    m_sample->Release();
    m_surface->Release();

    return (m_err) == S_OK ? true : false;
}

// FUNCTION: LEGORR 0x004722b0
bool G98CMovie::OpenStream(const char* fName, MovieAMStream** streamOut,
                           MovieUnknown* ddraw)
{
    MovieAMStream* stream = NULL;
    WCHAR wPath[MAX_PATH];
    HRESULT hr;

    hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL,
                          CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream,
                          (void**)&stream);
    if ((hr) == S_OK) {
        hr = stream->Initialize(MOVIE_STREAMTYPE_READ, 0, NULL);
        if ((hr) == S_OK) {
            hr = stream->AddMediaStream(ddraw, &MSPID_PrimaryVideo, 0,
                                        NULL);
            if ((hr) == S_OK) {
                hr = stream->AddMediaStream(NULL, &MSPID_PrimaryAudio,
                                            MOVIE_AMMSF_ADDDEFAULTRENDERER,
                                            NULL);
                if ((hr) != S_OK) hr = S_OK;
                MultiByteToWideChar(CP_ACP, 0, fName, -1, wPath, MAX_PATH);
                stream->OpenFile(wPath, 0);
                *streamOut = stream;
                stream->AddRef();
            }
        }
    }

    if (stream) stream->Release();

    return (hr) == S_OK ? true : false;
}

// FUNCTION: LEGORR 0x00472390
G98CMovie::G98CMovie(const char* fName, MovieSurface3* destSurf,
                     MovieUnknown* ddraw)
{
    /* The original leaves m_amStream and m_filename unset; they are read
     * when no stream opens. */
    m_amStream = NULL;
    m_filename = NULL;
    m_primaryVidStream = NULL;
    m_ddStream = NULL;
    m_surface = NULL;
    m_baseSurf = NULL;
    m_sample = NULL;
    m_destSurf = destSurf;
    m_ddraw = ddraw;

    CoInitialize(NULL);

    if (fName && strlen(fName)) {
        m_filename = (char*)malloc(strlen(fName) + 1);
        strcpy(m_filename, fName);
        if (OpenStream(fName, &m_amStream, ddraw)) InitSample(m_amStream);
    }
}

// FUNCTION: LEGORR 0x00472430
G98CMovie::~G98CMovie()
{
    if (m_primaryVidStream) m_primaryVidStream->Release();
    if (m_ddStream) m_ddStream->Release();
    if (m_sample) m_sample->Release();
    if (m_surface) m_surface->Release();
    if (m_destSurf) m_destSurf->Release();
    if (m_ddraw) m_ddraw->Release();
    if (m_amStream) m_amStream->Release();
    if (m_filename) free(m_filename);
}

// FUNCTION: LEGORR 0x004724a0
bool G98CMovie::Update(float speed, LPRECT destRect)
{
    if (m_sample) {
        if (m_sample->Update(0, NULL, NULL, 0) != S_OK) {
            m_primaryVidStream->Release();
            m_primaryVidStream = NULL;
            m_ddStream->Release();
            m_ddStream = NULL;
            m_sample->Release();
            m_sample = NULL;
            m_surface->Release();
            m_surface = NULL;
            if (m_amStream) {
                m_amStream->Release();
                m_amStream = NULL;
            }
            return false;
        }
        return (m_destSurf->Blt(destRect, m_baseSurf, NULL, MOVIE_DDBLT_WAIT,
                                NULL) == MOVIE_DD_OK) ? true : false;
    }

    return false;
}

// FUNCTION: LEGORR 0x00472530
unsigned int G98CMovie::GetDuration()
{
    LONGLONG duration;

    m_amStream->GetDuration(&duration);

    return (unsigned int)(duration / 10000);
}

/* Nothing calls this method, so the original's /OPT:REF removed it, but its
 * 64-bit multiply had already pulled LIBC's __allmul right after GetDuration's
 * __alldiv (finding 667). Its shape is inferred; only the reference
 * matters. */
// PORT-ONLY: the original's unreferenced position setter, which /OPT:REF removed; kept for its __allmul reference (finding 667)
void G98CMovie::SetPosition(unsigned int ms)
{
    m_amStream->Seek((LONGLONG)ms * 10000);
}

extern "C" {

// FUNCTION: LEGORR 0x00472560
G98CMovie* movie_load(const char* fName)
{
    const char* fullName = file_verify_filename(fName);
    G98CMovie* mov = NULL;
    MovieUnknown* ddraw;
    MovieSurface3* surf;
    char cdName[512];
    FILE* fp;

    /* The original hands DirectShow the IDirectDraw2 and the back buffer's
     * IDirectDrawSurface3. miniwin's objects are not COM-layout objects
     * that this unit's DirectShow views (MovieUnknown, MovieSurface3) can
     * call, and DirectShow is stubbed, so the movie gets neither: it opens
     * no stream, Update reports no frame, and the caller skips it. */
    ddraw = NULL; // [library:video]
    surf = NULL; // [library:video]

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
    MovieSurfaceDesc desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    mov->m_baseSurf->GetSurfaceDesc(&desc);
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

