// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-012 (text 0047e460-0047ed90; .rdata
 * 0049e9a0-0049e9b4, the two vtables and 0.001f; .bss 00533998-005339a0;
 * findings 505 and 604): gods98's AVI player. It is C++ in the original
 * (finding 505 rule 8): two classes with thiscall methods, vtables and EH
 * frames, and nine extern "C" entry points.
 *
 * - AviSurface (0x98 bytes, vtable 0049e9a0) wraps an IDirectDrawSurface4
 *   created from the IDirectDraw4 object that avi_initialise stores. Its
 *   DDSURFACEDESC2 is at +0x1c.
 * - AviMovie (0xc4 bytes, vtable 0049e9b0) opens a file's video stream
 *   through AVIFIL32, decodes frames with AVIStreamGetFrame and copies them
 *   into its AviSurface, which avi_blit_to_back_buffer blits to the back
 *   buffer.
 *
 * The binary attests none of the names (it has no RTTI); the classes and
 * functions use descriptive names.
 *
 * Windows only. The Windows builds (the matching build and the MinGW
 * reference build) compile the original's code, which calls DirectDraw
 * through its COM method tables and imports AVIFIL32 directly, as the
 * original does (finding 574). The native port has no DirectDraw or
 * AVIFIL32: there the file is empty, and the port's movie module
 * (movie.cpp, avi_player_port.cpp) plays movies through FFmpeg. */
#include "avi_player.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <string.h>
#include <windows.h>

#include "files.h"
#include "../game/aitask.h"
#include "heap.hpp"

/* AVIFIL32, called through the import thunks (finding 574; crt.cpp pairs
 * the original's six thunks with VFW32.LIB's). The unit declares the part
 * of the AVIFile API it uses rather than including MSVC 5.0's vfw.h: that
 * header defines `static DWORD dwICValue;`, which would add a word to the
 * unit's .bss that the original's (00533998-005339a0) does not have. */
struct AviStreamIface;                  /* IAVIStream */
struct AviGetFrameIface;                /* IGetFrame */

/* AVISTREAMINFOA (0x8c bytes). */
struct AviStreamInfoA {
    DWORD fccType;                      /* +0x00 */
    DWORD fccHandler;                   /* +0x04 */
    DWORD dwFlags;                      /* +0x08 */
    DWORD dwCaps;                       /* +0x0c */
    WORD wPriority;                     /* +0x10 */
    WORD wLanguage;                     /* +0x12 */
    DWORD dwScale;                      /* +0x14 */
    DWORD dwRate;                       /* +0x18 */
    DWORD dwStart;                      /* +0x1c */
    DWORD dwLength;                     /* +0x20 */
    DWORD dwInitialFrames;              /* +0x24 */
    DWORD dwSuggestedBufferSize;        /* +0x28 */
    DWORD dwQuality;                    /* +0x2c */
    DWORD dwSampleSize;                 /* +0x30 */
    RECT rcFrame;                       /* +0x34 */
    DWORD dwEditCount;                  /* +0x44 */
    DWORD dwFormatChangeCount;          /* +0x48 */
    char szName[64];                    /* +0x4c */
};

extern "C" {
void __stdcall AVIFileInit(void);
HRESULT __stdcall AVIStreamOpenFromFileA(AviStreamIface** ppavi,
                                         LPCSTR szFile, DWORD fccType,
                                         LONG lParam, UINT mode,
                                         void* pclsidHandler);
HRESULT __stdcall AVIStreamInfoA(AviStreamIface* pavi, AviStreamInfoA* psi,
                                 LONG lSize);
ULONG __stdcall AVIStreamRelease(AviStreamIface* pavi);
AviGetFrameIface* __stdcall AVIStreamGetFrameOpen(AviStreamIface* pavi,
                                                  LPBITMAPINFOHEADER lpbiWanted);
LPVOID __stdcall AVIStreamGetFrame(AviGetFrameIface* pg, LONG lPos);
}

#define AVI_STREAMTYPE_VIDEO            0x73646976  /* mmioFOURCC('v','i','d','s') */
#define AVI_GETFRAMEF_BESTDISPLAYFMT    1

/* The DirectX 6 interfaces the unit calls, with their method tables at the
 * DirectX 6 slots. MSVC 5.0's ddraw.h is DirectX 3's and has neither
 * IDirectDraw4 nor DDSURFACEDESC2, so the unit declares the layouts it
 * uses (as directdraw.c does). Unused slots are placeholders. */

/* DDPIXELFORMAT (0x20 bytes). */
struct AviPixelFormat {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
};

/* DDSURFACEDESC2 (0x7c bytes). */
struct AviSurfaceDesc2 {
    DWORD dwSize;                       /* +0x00 */
    DWORD dwFlags;                      /* +0x04 */
    DWORD dwHeight;                     /* +0x08 */
    DWORD dwWidth;                      /* +0x0c */
    LONG lPitch;                        /* +0x10 */
    DWORD dwBackBufferCount;            /* +0x14 */
    DWORD dwMipMapCount;                /* +0x18 */
    DWORD dwAlphaBitDepth;              /* +0x1c */
    DWORD dwReserved;                   /* +0x20 */
    LPVOID lpSurface;                   /* +0x24 */
    DWORD ddckCKDestOverlay[2];         /* +0x28 */
    DWORD ddckCKDestBlt[2];             /* +0x30 */
    DWORD ddckCKSrcOverlay[2];          /* +0x38 */
    DWORD ddckCKSrcBlt[2];              /* +0x40 */
    AviPixelFormat ddpfPixelFormat;     /* +0x48 */
    DWORD ddsCaps[4];                   /* +0x68 DDSCAPS2 */
    DWORD dwTextureStage;               /* +0x78 */
};

struct AviDirectDrawPalette;

/* IDirectDrawSurface4, all 45 slots. Deviation (scoring only): the unit
 * calls nothing after Unlock (slot 32), and the other interfaces stop at
 * the last slot they call. The surface's full table is declared because
 * those 12 declarations put AviSurface::Init (0x0047e510) in the phase of
 * MSVC 5.0's symbol counter that the original's (finding 538 rule B):
 * 98.76% without them, 100% with them. It changes no storage. */
struct AviDirectDrawSurface4 {
    virtual HRESULT __stdcall QueryInterface(void*, void**) = 0;
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;
    virtual HRESULT __stdcall AddAttachedSurface(void*) = 0;
    virtual HRESULT __stdcall AddOverlayDirtyRect(LPRECT) = 0;
    virtual HRESULT __stdcall Blt(LPRECT, AviDirectDrawSurface4*, LPRECT,
                                  DWORD, void*) = 0;
    virtual HRESULT __stdcall BltBatch(void*, DWORD, DWORD) = 0;
    virtual HRESULT __stdcall BltFast(DWORD, DWORD, void*, LPRECT,
                                      DWORD) = 0;
    virtual HRESULT __stdcall DeleteAttachedSurface(DWORD, void*) = 0;
    virtual HRESULT __stdcall EnumAttachedSurfaces(void*, void*) = 0;
    virtual HRESULT __stdcall EnumOverlayZOrders(DWORD, void*, void*) = 0;
    virtual HRESULT __stdcall Flip(void*, DWORD) = 0;
    virtual HRESULT __stdcall GetAttachedSurface(void*, void**) = 0;
    virtual HRESULT __stdcall GetBltStatus(DWORD) = 0;
    virtual HRESULT __stdcall GetCaps(void*) = 0;
    virtual HRESULT __stdcall GetClipper(void**) = 0;
    virtual HRESULT __stdcall GetColorKey(DWORD, void*) = 0;
    virtual HRESULT __stdcall GetDC(HDC*) = 0;
    virtual HRESULT __stdcall GetFlipStatus(DWORD) = 0;
    virtual HRESULT __stdcall GetOverlayPosition(LPLONG, LPLONG) = 0;
    virtual HRESULT __stdcall GetPalette(AviDirectDrawPalette**) = 0;
    virtual HRESULT __stdcall GetPixelFormat(AviPixelFormat*) = 0;
    virtual HRESULT __stdcall GetSurfaceDesc(AviSurfaceDesc2*) = 0;
    virtual HRESULT __stdcall Initialize(void*, AviSurfaceDesc2*) = 0;
    virtual HRESULT __stdcall IsLost() = 0;
    virtual HRESULT __stdcall Lock(LPRECT, AviSurfaceDesc2*, DWORD,
                                   HANDLE) = 0;
    virtual HRESULT __stdcall ReleaseDC(HDC) = 0;
    virtual HRESULT __stdcall Restore() = 0;
    virtual HRESULT __stdcall SetClipper(void*) = 0;
    virtual HRESULT __stdcall SetColorKey(DWORD, void*) = 0;
    virtual HRESULT __stdcall SetOverlayPosition(LONG, LONG) = 0;
    virtual HRESULT __stdcall SetPalette(AviDirectDrawPalette*) = 0;
    virtual HRESULT __stdcall Unlock(LPRECT) = 0;
    virtual HRESULT __stdcall UpdateOverlay(LPRECT, AviDirectDrawSurface4*, LPRECT, DWORD, void*) = 0;
    virtual HRESULT __stdcall UpdateOverlayDisplay(DWORD) = 0;
    virtual HRESULT __stdcall UpdateOverlayZOrder(DWORD, AviDirectDrawSurface4*) = 0;
    virtual HRESULT __stdcall GetDDInterface(void**) = 0;
    virtual HRESULT __stdcall PageLock(DWORD) = 0;
    virtual HRESULT __stdcall PageUnlock(DWORD) = 0;
    virtual HRESULT __stdcall SetSurfaceDesc(AviSurfaceDesc2*, DWORD) = 0;
    virtual HRESULT __stdcall SetPrivateData(const GUID&, void*, DWORD, DWORD) = 0;
    virtual HRESULT __stdcall GetPrivateData(const GUID&, void*, LPDWORD) = 0;
    virtual HRESULT __stdcall FreePrivateData(const GUID&) = 0;
    virtual HRESULT __stdcall GetUniquenessValue(LPDWORD) = 0;
    virtual HRESULT __stdcall ChangeUniquenessValue() = 0;
};

/* IDirectDrawPalette, up to Release (slot 2). */
struct AviDirectDrawPalette {
    virtual HRESULT __stdcall QueryInterface(void*, void**) = 0;
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;
};

/* IDirectDraw4, up to CreateSurface (slot 6). */
struct AviDirectDraw4 {
    virtual HRESULT __stdcall QueryInterface(void*, void**) = 0;
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;
    virtual HRESULT __stdcall Compact() = 0;
    virtual HRESULT __stdcall CreateClipper(DWORD, void**, void*) = 0;
    virtual HRESULT __stdcall CreatePalette(DWORD, LPPALETTEENTRY,
                                            AviDirectDrawPalette**,
                                            void*) = 0;
    virtual HRESULT __stdcall CreateSurface(AviSurfaceDesc2*,
                                            AviDirectDrawSurface4**,
                                            void*) = 0;
};

#define AVI_DDSD_CAPS               0x00000001
#define AVI_DDSD_HEIGHT             0x00000002
#define AVI_DDSD_WIDTH              0x00000004
#define AVI_DDSD_PIXELFORMAT        0x00001000
#define AVI_DDSCAPS_OFFSCREENPLAIN  0x00000040
#define AVI_DDSCAPS_PRIMARYSURFACE  0x00000200
#define AVI_DDSCAPS_SYSTEMMEMORY    0x00000800
#define AVI_DDSCAPS_VIDEOMEMORY     0x00004000
#define AVI_DDPF_PALETTEINDEXED8    0x00000020
#define AVI_DDPF_RGB                0x00000040
#define AVI_DDPCAPS_8BIT            0x00000004
#define AVI_DDLOCK_WAIT             0x00000001
#define AVI_DDLOCK_NOSYSLOCK        0x00000800
#define AVI_DDBLT_WAIT              0x01000000
#define AVI_DD_OK                   0

/* Callees declared by their own headers: aitask.h gives the back buffer
 * (render_surface_secondary_get, 0x004065e0, AITask.c's out-of-line copy of
 * the DirectDraw_bSurf accessor); heap.hpp gives the native port's operator
 * new (engine_calloc; the matching build calls LIBC's, 0x0048ead0) and
 * operator delete (ui_render_record_free, 0x0045daf0); files.h gives
 * File_VerifyFilename. */

#if defined(_MSC_VER)
#define AVI_OPERATOR_NEW(size) ::operator new(size)
#else
#define AVI_OPERATOR_NEW(size) engine_calloc((long)(size))
#endif

/* Read by every AviMovie method; nothing in the image writes it. */
// GLOBAL: LEGORR 0x00533998
static bool s_aviDisabled;

// GLOBAL: LEGORR 0x0053399c
static AviDirectDraw4* s_aviDirectDraw;

// VTABLE: LEGORR 0x0049e9a0
class AviSurface {
public:
    AviSurface(long width, long height, long bpp, bool vram, bool unused);
    virtual ~AviSurface();
    virtual long Width();
    virtual long Height();
    bool Lock();
    bool Unlock();

    /* Deviation (scoring only): the original's new-expressions and
     * destructors call the global operator new (LIBC's, 0x0048ead0) and
     * delete (0x0045daf0). The port defines no global operator delete under
     * that name, so class-scope operators forward to the functions that
     * carry the annotations; the matching build takes operator new from
     * LIBC, the native port from engine_calloc. Inline, they compile to the
     * original's direct calls. */
    // PORT-ONLY: routes new to the original's operator new (0x0048ead0)
    static void* operator new(size_t size) { return AVI_OPERATOR_NEW(size); }
    // PORT-ONLY: routes delete to the original's operator delete (0x0045daf0)
    static void operator delete(void* p) { ui_render_record_free(p); }

private:
    bool Init(long width, long height, long bpp, bool vram);
    void InitDesc(bool unused);
    long CountMaskBits(unsigned long mask);

public:
    unsigned long m_unused04;           /* +0x04 */
    bool m_locked;                      /* +0x08 */
    long m_bpp;                         /* +0x0c */
    AviDirectDrawPalette* m_palette;    /* +0x10 */
    bool m_unused14;                    /* +0x14 */
    bool m_unused15;                    /* +0x15 */
    bool m_is555;                       /* +0x16 */
    bool m_ok;                          /* +0x17 */
    AviDirectDrawSurface4* m_surf;      /* +0x18 */
    AviSurfaceDesc2 m_desc;             /* +0x1c */
};

// VTABLE: LEGORR 0x0049e9b0
class AviMovie {
public:
    AviMovie(char* fName);
    virtual ~AviMovie();
    void FrameToSurface(LPBITMAPINFOHEADER lpbi);
    bool Update();
    unsigned long GetLength();
    void SetTime(unsigned long time);
    unsigned long GetTime();

    /* Deviation (scoring only): see AviSurface. */
    // PORT-ONLY: routes new to the original's operator new (0x0048ead0)
    static void* operator new(size_t size) { return AVI_OPERATOR_NEW(size); }
    // PORT-ONLY: routes delete to the original's operator delete (0x0045daf0)
    static void operator delete(void* p) { ui_render_record_free(p); }

    AviStreamIface* m_aviStream;        /* +0x04 */
    AviGetFrameIface* m_getFrame;       /* +0x08 */
    AviStreamInfoA m_info;              /* +0x0c */
    bool m_init;                        /* +0x98 */
    bool m_ok;                          /* +0x99 */
    RECT m_rect;                        /* +0x9c */
    float m_startTime;                  /* +0xac */
    float m_currTime;                   /* +0xb0 */
    float m_frameRate;                  /* +0xb4 */
    unsigned long m_currFrame;          /* +0xb8 */
    unsigned long m_unusedBc;           /* +0xbc */
    AviSurface* m_surf;                 /* +0xc0 */
};

// FUNCTION: LEGORR 0x0047e460
// AviSurface::`scalar deleting destructor'

// FUNCTION: LEGORR 0x0047e480
AviSurface::~AviSurface()
{
    if (m_surf) {
        m_surf->Release();
        m_surf = NULL;
    }
    if (m_palette) {
        m_palette->Release();
        m_palette = NULL;
    }
}

// FUNCTION: LEGORR 0x0047e4c0
AviSurface::AviSurface(long width, long height, long bpp, bool vram, bool unused)
{
    m_unused04 = 0;
    m_unused15 = false;
    m_is555 = false;
    m_surf = NULL;
    m_ok = false;
    InitDesc(unused);
    if (Init(width, height, bpp, vram)) m_ok = true;
}

// FUNCTION: LEGORR 0x0047e510
bool AviSurface::Init(long width, long height, long bpp, bool vram)
{
    PALETTEENTRY pal[256];
    long i;

    m_desc.dwFlags |= (width ? AVI_DDSD_WIDTH : 0) | (height ? AVI_DDSD_HEIGHT : 0) | AVI_DDSD_CAPS;
    m_bpp = bpp;
    if (width) m_desc.dwWidth = width;
    if (height) m_desc.dwHeight = height;

    if (!width && !height) {
        m_desc.ddsCaps[0] = AVI_DDSCAPS_PRIMARYSURFACE;
    } else {
        m_desc.ddsCaps[0] = AVI_DDSCAPS_OFFSCREENPLAIN;
        if (vram) m_desc.ddsCaps[0] |= AVI_DDSCAPS_VIDEOMEMORY; else m_desc.ddsCaps[0] |= AVI_DDSCAPS_SYSTEMMEMORY;
    }

    if (bpp) {
        m_desc.dwFlags |= AVI_DDSD_PIXELFORMAT;
        m_desc.ddpfPixelFormat.dwSize = sizeof(AviPixelFormat);
        m_desc.ddpfPixelFormat.dwRGBBitCount = (bpp == 15) ? 16 : bpp;
        if (bpp == 24) {
            m_desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
            m_desc.ddpfPixelFormat.dwGBitMask = 0xff00;
            m_desc.ddpfPixelFormat.dwBBitMask = 0xff;
            m_desc.ddpfPixelFormat.dwFlags = AVI_DDPF_RGB;
        } else if (bpp == 16) {
            m_desc.ddpfPixelFormat.dwRBitMask = 0xf800;
            m_desc.ddpfPixelFormat.dwGBitMask = 0x7e0;
            m_desc.ddpfPixelFormat.dwBBitMask = 0x1f;
            m_desc.ddpfPixelFormat.dwFlags = AVI_DDPF_RGB;
        } else if (bpp == 15) {
            m_desc.ddpfPixelFormat.dwRBitMask = 0x7c00;
            m_desc.ddpfPixelFormat.dwGBitMask = 0x3e0;
            m_desc.ddpfPixelFormat.dwBBitMask = 0x1f;
            m_desc.ddpfPixelFormat.dwFlags = AVI_DDPF_RGB;
        } else if (bpp == 8) {
            m_desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
            m_desc.ddpfPixelFormat.dwGBitMask = 0xff00;
            m_desc.ddpfPixelFormat.dwBBitMask = 0xff;
            m_desc.ddpfPixelFormat.dwFlags = AVI_DDPF_RGB | AVI_DDPF_PALETTEINDEXED8;
        }
    }

    if (s_aviDirectDraw->CreateSurface(&m_desc, &m_surf, NULL) == AVI_DD_OK) {
        Lock();
        Unlock();
        if (m_desc.ddpfPixelFormat.dwRGBBitCount == 8) {
            for (i = 0; i < 256; i++) {
                pal[i].peRed = 0;
                pal[i].peGreen = 0;
                pal[i].peBlue = 0;
                pal[i].peFlags = 0;
            }
            if (s_aviDirectDraw->CreatePalette(AVI_DDPCAPS_8BIT, pal, &m_palette, NULL) != AVI_DD_OK) return false;
            if (m_surf->SetPalette(m_palette) != AVI_DD_OK) return false;
        }
        if (m_desc.ddpfPixelFormat.dwRGBBitCount == 16) {
            if (CountMaskBits(m_desc.ddpfPixelFormat.dwGBitMask) +
                CountMaskBits(m_desc.ddpfPixelFormat.dwBBitMask) +
                CountMaskBits(m_desc.ddpfPixelFormat.dwRBitMask) == 15) m_is555 = true;
        }
        return true;
    }
    return false;
}

// FUNCTION: LEGORR 0x0047e710
void AviSurface::InitDesc(bool unused)
{
    memset(&m_desc, 0, sizeof(m_desc));
    m_desc.dwSize = sizeof(m_desc);
    m_surf = NULL;
    m_palette = NULL;
    m_locked = false;
    m_unused14 = false;
}

// FUNCTION: LEGORR 0x0047e740
long AviSurface::Width()
{
    return m_desc.dwWidth;
}

// FUNCTION: LEGORR 0x0047e750
long AviSurface::Height()
{
    return m_desc.dwHeight;
}

// FUNCTION: LEGORR 0x0047e760
bool AviSurface::Lock()
{
    if (!m_locked) {
        memset(&m_desc, 0, sizeof(m_desc));
        m_desc.dwSize = sizeof(m_desc);
        if (m_surf->Lock(NULL, &m_desc, AVI_DDLOCK_WAIT | AVI_DDLOCK_NOSYSLOCK, NULL) == AVI_DD_OK) {
            m_locked = true;
            return true;
        }
    }
    return false;
}

// FUNCTION: LEGORR 0x0047e7b0
bool AviSurface::Unlock()
{
    if (m_locked) {
        if (m_surf->Unlock(NULL) == AVI_DD_OK) {
            m_locked = false;
            return true;
        }
    }
    return false;
}

// FUNCTION: LEGORR 0x0047e7e0
long AviSurface::CountMaskBits(unsigned long mask)
{
    long count = 0;
    unsigned long bit = 0;
    do {
        if (mask & (1 << bit)) count++;
        bit++;
    } while (bit < 32);
    return count;
}

// FUNCTION: LEGORR 0x0047e800
AviMovie::AviMovie(char* fName) : m_init(false), m_ok(false), m_surf(NULL)
{
    fName = file_verify_filename(fName);
    m_getFrame = NULL;
    m_aviStream = NULL;
    memset(&m_info, 0, sizeof(m_info));
    m_rect.top = 0;
    m_rect.bottom = 0;
    m_rect.left = 0;
    m_rect.right = 0;

    if (!s_aviDisabled) {
        if (AVIStreamOpenFromFileA(&m_aviStream, fName, AVI_STREAMTYPE_VIDEO, 0, OF_READ, NULL) == 0) {
            if ((m_getFrame = AVIStreamGetFrameOpen(m_aviStream, (LPBITMAPINFOHEADER)AVI_GETFRAMEF_BESTDISPLAYFMT)) != NULL) {
                if (AVIStreamInfoA(m_aviStream, &m_info, sizeof(m_info)) == 0) {
                    m_startTime = timeGetTime() * 0.001f;
                    m_ok = true;
                    m_currFrame = 0;
                    m_rect.top = m_info.rcFrame.top;
                    m_rect.bottom = m_info.rcFrame.bottom;
                    m_rect.left = m_info.rcFrame.left;
                    m_rect.right = m_info.rcFrame.right;
                    m_surf = new AviSurface(m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, 0, false, false);
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0047e980
// AviMovie::`scalar deleting destructor'

// FUNCTION: LEGORR 0x0047e9a0
AviMovie::~AviMovie()
{
    if (!s_aviDisabled) {
        if (m_aviStream) AVIStreamRelease(m_aviStream);
        delete m_surf;
    }
}

// FUNCTION: LEGORR 0x0047e9e0
void AviMovie::FrameToSurface(LPBITMAPINFOHEADER lpbi)
{
    bool locked = false;
    AviSurface* surf = m_surf;
    unsigned short* dst;
    unsigned short* src;
    long width, height, pitch, y;
    long bpp;

    if (!surf->m_locked && surf->Lock()) locked = true;

    dst = (unsigned short*)surf->m_desc.lpSurface;
    width = m_rect.right - m_rect.left;
    height = m_rect.bottom - m_rect.top;
    pitch = m_surf->m_desc.lPitch / 2;
    src = (unsigned short*)((char*)lpbi + lpbi->biSize);

    bpp = m_surf->m_is555 ? 15 : m_surf->m_desc.ddpfPixelFormat.dwRGBBitCount;
    if (bpp == 15) {
        src += (height - 1) * pitch;
        for (y = 0; y < height; y++) {
            memcpy(dst, src, width * 2);
            src -= pitch;
            dst += pitch;
        }
    } else {
        bpp = m_surf->m_is555 ? 15 : m_surf->m_desc.ddpfPixelFormat.dwRGBBitCount;
        if (bpp == 16) {
            src += (height - 1) * pitch;
            for (y = 0; y < height; y++) {
                memcpy(dst, src, width * 2);
                src -= pitch;
                dst += pitch;
            }
        }
    }

    if (locked) surf->Unlock();
}

// FUNCTION: LEGORR 0x0047eb20
bool AviMovie::Update()
{
    LPBITMAPINFOHEADER lpbi;

    if (!m_ok) return false;
    if (s_aviDisabled) return false;

    if ((lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(m_getFrame, m_currFrame)) != NULL) {
        FrameToSurface(lpbi);
        DWORD time = timeGetTime();
        float rate = (float)m_info.dwRate, scale = (float)m_info.dwScale;
        m_frameRate = rate / scale;
        m_currTime = time * 0.001f;
        m_currFrame = (unsigned long)((m_currTime - m_startTime) * m_frameRate);
    }
    return true;
}

// FUNCTION: LEGORR 0x0047ebe0
unsigned long AviMovie::GetLength()
{
    if (s_aviDisabled) return 0;
    return m_info.dwLength;
}

// FUNCTION: LEGORR 0x0047ebf0
void AviMovie::SetTime(unsigned long time)
{
    if (!s_aviDisabled) {
        m_currFrame = time;
        m_currTime = timeGetTime() * 0.001f;
        m_startTime = m_currTime - (float)m_currFrame / m_frameRate;
    }
}

// FUNCTION: LEGORR 0x0047ec60
unsigned long AviMovie::GetTime()
{
    unsigned long frame = m_currFrame;

    if (s_aviDisabled) return 0;
    return frame;
}

extern "C" {

// FUNCTION: LEGORR 0x0047ec80
void avi_initialise(void* directDraw)
{
    s_aviDirectDraw = (AviDirectDraw4*)directDraw;
    AVIFileInit();
}

// FUNCTION: LEGORR 0x0047ec90
AviMovie* avi_load(char* fName)
{
    return new AviMovie(fName);
}

// FUNCTION: LEGORR 0x0047ecf0
int avi_update(AviMovie* anim)
{
    return anim->Update();
}

// FUNCTION: LEGORR 0x0047ed00
void avi_blit_to_back_buffer(AviMovie* anim, struct tagRECT* destRect)
{
    if (anim->m_ok) {
        AviDirectDrawSurface4* surf = anim->m_surf->m_surf;
        ((AviDirectDrawSurface4*)render_surface_secondary_get())->Blt(destRect, surf, NULL, AVI_DDBLT_WAIT, NULL);
    }
}

// FUNCTION: LEGORR 0x0047ed40
void avi_set_time(AviMovie* anim, unsigned long time)
{
    anim->SetTime(time);
}

// FUNCTION: LEGORR 0x0047ed50
unsigned long avi_get_time(AviMovie* anim)
{
    return anim->GetTime();
}

// FUNCTION: LEGORR 0x0047ed60
unsigned long avi_get_length(AviMovie* anim)
{
    return anim->GetLength();
}

// FUNCTION: LEGORR 0x0047ed70
void avi_remove(AviMovie* anim)
{
    delete anim;
}

// FUNCTION: LEGORR 0x0047ed80
int avi_is_ok(AviMovie* anim)
{
    return anim->m_ok;
}

} /* extern "C" */

#endif /* !LEGORRR_BACKEND_MINIWIN */
