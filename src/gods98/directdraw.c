// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-010 (text 0047c170-0047d410; .data strings
 * 004aae64-004aae7c; the common directDrawGlobs at 0076ac80; findings 505
 * and 596): gods98's DirectDraw.c. It enumerates the DirectDraw drivers,
 * their Direct3D devices and display modes, sets up the display (the
 * DirectDraw object, the front and back buffers and their clippers), flips
 * and clears the buffers, saves a surface as a bitmap, and converts
 * palettised surfaces. The binary attests none of the names; the functions
 * use descriptive DirectDraw_* names in snake case.
 *
 * Windows only. The Windows builds (the matching build and the MinGW
 * reference build) compile the original's code, which calls DirectDraw
 * through its COM method tables and imports DirectDrawCreate and
 * DirectDrawEnumerateA as the original does (finding 574). The native port
 * has no DirectDraw: there the file is empty, and directdraw_port.cpp gives
 * the entry points that converted units call (and the port renderer) their
 * native bodies.
 *
 * C-compatible (ADR 0005): C in the matching build, C++ in the MinGW
 * reference build. */
#include "directdraw.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "crt/vc5names.h"
#include "files.h"
#include "images.h"
#include "directdraw_imports.h"

/* The COM interfaces the unit calls, with their method tables at the
 * DirectX 6 slots. MSVC 5.0's ddraw.h is DirectX 3's and has neither
 * IDirectDraw4 nor DDSURFACEDESC2, so the unit declares the layouts it
 * uses. Unused slots are placeholders. */
typedef struct DirectDrawSurfaceDesc2 DirectDrawSurfaceDesc2;
typedef struct DirectDrawIface1 DirectDrawIface1;          /* IDirectDraw */
typedef struct DirectDrawIface DirectDrawIface;            /* IDirectDraw4 */
typedef struct DirectDrawSurfaceIface DirectDrawSurfaceIface; /* IDirectDrawSurface4 */
typedef struct DirectDrawClipperIface DirectDrawClipperIface; /* IDirectDrawClipper */
typedef struct DirectDrawPaletteIface DirectDrawPaletteIface; /* IDirectDrawPalette */
typedef struct Direct3DIface Direct3DIface;                /* IDirect3D3 */
typedef struct Direct3DRMDeviceIface Direct3DRMDeviceIface; /* IDirect3DRMDevice3 */

/* DDPIXELFORMAT (0x20 bytes). */
typedef struct DirectDrawPixelFormat {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
} DirectDrawPixelFormat;

/* DDSCAPS2 (0x10 bytes). */
typedef struct DirectDrawCaps2 {
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
} DirectDrawCaps2;

/* DDSURFACEDESC2 (0x7c bytes). */
struct DirectDrawSurfaceDesc2 {
    DWORD dwSize;                           /* +0x00 */
    DWORD dwFlags;                          /* +0x04 */
    DWORD dwHeight;                         /* +0x08 */
    DWORD dwWidth;                          /* +0x0c */
    LONG lPitch;                            /* +0x10 */
    DWORD dwBackBufferCount;                /* +0x14 */
    DWORD dwRefreshRate;                    /* +0x18 */
    DWORD dwAlphaBitDepth;                  /* +0x1c */
    DWORD dwReserved;                       /* +0x20 */
    void* lpSurface;                        /* +0x24 */
    DWORD ddckCKDestOverlay[2];             /* +0x28 */
    DWORD ddckCKDestBlt[2];                 /* +0x30 */
    DWORD ddckCKSrcOverlay[2];              /* +0x38 */
    DWORD ddckCKSrcBlt[2];                  /* +0x40 */
    DirectDrawPixelFormat ddpfPixelFormat;  /* +0x48 */
    DirectDrawCaps2 ddsCaps;                /* +0x68 */
    DWORD dwTextureStage;                   /* +0x78 */
};

/* DDBLTFX (0x64 bytes); dwFillColor at +0x50. */
typedef struct DirectDrawBltFx {
    DWORD dwSize;
    DWORD reserved[19];
    DWORD dwFillColor;
    DWORD reserved2[4];
} DirectDrawBltFx;

/* D3DDEVICEDESC (DirectX 6, 0xfc bytes): the fields the device callback
 * reads. */
typedef struct Direct3DDeviceDesc {
    DWORD dwSize;                           /* +0x00 */
    DWORD dwFlags;                          /* +0x04 */
    DWORD dcmColorModel;                    /* +0x08 */
    DWORD dwDevCaps;                        /* +0x0c */
    DWORD reserved[35];                     /* +0x10 */
    DWORD dwDeviceRenderBitDepth;           /* +0x9c */
} Direct3DDeviceDesc;

/* PALETTEENTRY. */
typedef struct DirectDrawPaletteEntry {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} DirectDrawPaletteEntry;

typedef BOOL (WINAPI* DirectDrawEnumDriverCallback)(
    DirectDraw_Guid*, LPSTR, LPSTR, LPVOID);
typedef HRESULT (WINAPI* DirectDrawEnumDeviceCallback)(
    DirectDraw_Guid*, LPSTR, LPSTR, Direct3DDeviceDesc*,
    Direct3DDeviceDesc*, LPVOID);
typedef HRESULT (WINAPI* DirectDrawEnumModeCallback)(
    DirectDrawSurfaceDesc2*, LPVOID);

typedef struct DirectDrawIface1Vtbl {
    HRESULT (WINAPI* QueryInterface)(DirectDrawIface1*,
                                     const DirectDraw_Guid*, void**);
    ULONG (WINAPI* AddRef)(DirectDrawIface1*);
    ULONG (WINAPI* Release)(DirectDrawIface1*);
} DirectDrawIface1Vtbl;

struct DirectDrawIface1 {
    const DirectDrawIface1Vtbl* lpVtbl;
};

typedef struct DirectDrawIfaceVtbl {
    HRESULT (WINAPI* QueryInterface)(DirectDrawIface*,
                                     const DirectDraw_Guid*, void**);
    ULONG (WINAPI* AddRef)(DirectDrawIface*);
    ULONG (WINAPI* Release)(DirectDrawIface*);
    void* Compact;
    HRESULT (WINAPI* CreateClipper)(DirectDrawIface*, DWORD,
                                    DirectDrawClipperIface**, void*);
    void* CreatePalette;
    HRESULT (WINAPI* CreateSurface)(DirectDrawIface*, DirectDrawSurfaceDesc2*,
                                    DirectDrawSurfaceIface**, void*);
    void* DuplicateSurface;
    HRESULT (WINAPI* EnumDisplayModes)(DirectDrawIface*, DWORD,
                                       DirectDrawSurfaceDesc2*, LPVOID,
                                       DirectDrawEnumModeCallback);
    void* EnumSurfaces;
    void* FlipToGDISurface;
    void* GetCaps;
    void* GetDisplayMode;
    void* GetFourCCCodes;
    void* GetGDISurface;
    void* GetMonitorFrequency;
    void* GetScanLine;
    void* GetVerticalBlankStatus;
    void* Initialize;
    HRESULT (WINAPI* RestoreDisplayMode)(DirectDrawIface*);
    HRESULT (WINAPI* SetCooperativeLevel)(DirectDrawIface*, HWND, DWORD);
    HRESULT (WINAPI* SetDisplayMode)(DirectDrawIface*, DWORD, DWORD, DWORD,
                                     DWORD, DWORD);
    void* WaitForVerticalBlank;
    HRESULT (WINAPI* GetAvailableVidMem)(DirectDrawIface*, DirectDrawCaps2*,
                                         unsigned int*, unsigned int*);
} DirectDrawIfaceVtbl;

struct DirectDrawIface {
    const DirectDrawIfaceVtbl* lpVtbl;
};

typedef struct DirectDrawSurfaceIfaceVtbl {
    void* QueryInterface;
    ULONG (WINAPI* AddRef)(DirectDrawSurfaceIface*);
    ULONG (WINAPI* Release)(DirectDrawSurfaceIface*);
    void* AddAttachedSurface;
    void* AddOverlayDirtyRect;
    HRESULT (WINAPI* Blt)(DirectDrawSurfaceIface*, RECT*,
                          DirectDrawSurfaceIface*, RECT*, DWORD,
                          DirectDrawBltFx*);
    void* BltBatch;
    void* BltFast;
    void* DeleteAttachedSurface;
    void* EnumAttachedSurfaces;
    void* EnumOverlayZOrders;
    HRESULT (WINAPI* Flip)(DirectDrawSurfaceIface*, DirectDrawSurfaceIface*,
                           DWORD);
    HRESULT (WINAPI* GetAttachedSurface)(DirectDrawSurfaceIface*,
                                         DirectDrawCaps2*,
                                         DirectDrawSurfaceIface**);
    void* GetBltStatus;
    void* GetCaps;
    void* GetClipper;
    void* GetColorKey;
    void* GetDC;
    void* GetFlipStatus;
    void* GetOverlayPosition;
    HRESULT (WINAPI* GetPalette)(DirectDrawSurfaceIface*,
                                 DirectDrawPaletteIface**);
    HRESULT (WINAPI* GetPixelFormat)(DirectDrawSurfaceIface*,
                                     DirectDrawPixelFormat*);
    void* GetSurfaceDesc;
    void* Initialize;
    void* IsLost;
    HRESULT (WINAPI* Lock)(DirectDrawSurfaceIface*, RECT*,
                           DirectDrawSurfaceDesc2*, DWORD, HANDLE);
    void* ReleaseDC;
    void* Restore;
    HRESULT (WINAPI* SetClipper)(DirectDrawSurfaceIface*,
                                 DirectDrawClipperIface*);
    void* SetColorKey;
    void* SetOverlayPosition;
    void* SetPalette;
    HRESULT (WINAPI* Unlock)(DirectDrawSurfaceIface*, RECT*);
} DirectDrawSurfaceIfaceVtbl;

struct DirectDrawSurfaceIface {
    const DirectDrawSurfaceIfaceVtbl* lpVtbl;
};

typedef struct DirectDrawClipperIfaceVtbl {
    void* QueryInterface;
    ULONG (WINAPI* AddRef)(DirectDrawClipperIface*);
    ULONG (WINAPI* Release)(DirectDrawClipperIface*);
    void* GetClipList;
    void* GetHWnd;
    void* Initialize;
    void* IsClipListChanged;
    HRESULT (WINAPI* SetClipList)(DirectDrawClipperIface*, RGNDATA*, DWORD);
    HRESULT (WINAPI* SetHWnd)(DirectDrawClipperIface*, DWORD, HWND);
} DirectDrawClipperIfaceVtbl;

struct DirectDrawClipperIface {
    const DirectDrawClipperIfaceVtbl* lpVtbl;
};

typedef struct DirectDrawPaletteIfaceVtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* GetCaps;
    HRESULT (WINAPI* GetEntries)(DirectDrawPaletteIface*, DWORD, DWORD,
                                 DWORD, DirectDrawPaletteEntry*);
} DirectDrawPaletteIfaceVtbl;

struct DirectDrawPaletteIface {
    const DirectDrawPaletteIfaceVtbl* lpVtbl;
};

typedef struct Direct3DIfaceVtbl {
    void* QueryInterface;
    void* AddRef;
    ULONG (WINAPI* Release)(Direct3DIface*);
    HRESULT (WINAPI* EnumDevices)(Direct3DIface*,
                                  DirectDrawEnumDeviceCallback, LPVOID);
} Direct3DIfaceVtbl;

struct Direct3DIface {
    const Direct3DIfaceVtbl* lpVtbl;
};

/* IDirect3DRMDevice3, up to FindPreferredTextureFormat (slot 39). */
typedef struct Direct3DRMDeviceIfaceVtbl {
    void* slots[39];
    HRESULT (WINAPI* FindPreferredTextureFormat)(Direct3DRMDeviceIface*,
                                                 DWORD, DWORD,
                                                 DirectDrawPixelFormat*);
} Direct3DRMDeviceIfaceVtbl;

struct Direct3DRMDeviceIface {
    const Direct3DRMDeviceIfaceVtbl* lpVtbl;
};

/* DDRAW.DLL, called through the import thunks (finding 574; crt.cpp
 * pairs the original's two thunks with the import library's). */
#ifdef __cplusplus
extern "C" {
#endif
HRESULT WINAPI DirectDrawCreate(DirectDraw_Guid* lpGUID,
                                DirectDrawIface1** lplpDD, void* pUnkOuter);
HRESULT WINAPI DirectDrawEnumerateA(DirectDrawEnumDriverCallback lpCallback,
                                    LPVOID lpContext);
#ifdef __cplusplus
}
#endif

/* The BMP file and info headers, packed (0x36 bytes). */
#pragma pack(1)
typedef struct DirectDrawBmpHeader {
    char bfType[2];
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} DirectDrawBmpHeader;
#pragma pack()

/* Main.c's device (lpDevice()). */
#define lpDevice() ((Direct3DRMDeviceIface*)s_main_device)

#define DD_OK 0
#define DDSCL_FULLSCREEN 0x00000001
#define DDSCL_NORMAL 0x00000008
#define DDSCL_EXCLUSIVE 0x00000010
#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSD_BACKBUFFERCOUNT 0x00000020
#define DDSCAPS_BACKBUFFER 0x00000004
#define DDSCAPS_COMPLEX 0x00000008
#define DDSCAPS_FLIP 0x00000010
#define DDSCAPS_OFFSCREENPLAIN 0x00000040
#define DDSCAPS_PRIMARYSURFACE 0x00000200
#define DDSCAPS_TEXTURE 0x00001000
#define DDSCAPS_3DDEVICE 0x00002000
#define DDFLIP_WAIT 0x00000001
#define DDBLT_COLORFILL 0x00000400
#define DDBLT_WAIT 0x01000000
#define DDLOCK_WAIT 0x00000001
#define DDPF_RGB 0x00000040
#define DDBD_8 0x00000800
#define DDBD_16 0x00000400
#define DDBD_24 0x00000200
#define DDBD_32 0x00000100
#define D3DDD_COLORMODEL 0x00000001
#define D3DDD_DEVCAPS 0x00000002
#define D3DDD_DEVICERENDERBITDEPTH 0x00000080
#define D3DCOLOR_RGB 2
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY 0x00000100
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY 0x00000200
#define D3DRMFPTF_PALETTIZED 0x00000004

static BOOL WINAPI directdraw_enum_driver_callback(DirectDraw_Guid* lpGUID,
    LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
static HRESULT WINAPI directdraw_enum_device_callback(DirectDraw_Guid* lpGuid,
    LPSTR lpDeviceDescription, LPSTR lpDeviceName,
    Direct3DDeviceDesc* lpHWDesc, Direct3DDeviceDesc* lpHELDesc,
    LPVOID lpContext);
static HRESULT WINAPI directdraw_enum_mode_callback(
    DirectDrawSurfaceDesc2* lpDDSurfaceDesc, LPVOID lpContext);

/* IID_IDirectDraw4 and IID_IDirect3D3. The original links them from
 * dxguid.lib; as in sound3d.c and lws.c, the unit defines its own copies. */
// GLOBAL: LEGORR 0x0049f328
static const DirectDraw_Guid s_iid_direct_draw4 = {
    0x9c59509a, 0x39bd, 0x11d1,
    { 0x8c, 0x4a, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc5 }
};

// GLOBAL: LEGORR 0x0049f648
static const DirectDraw_Guid s_iid_direct3d3 = {
    0xbb223240, 0xe72b, 0x11d0,
    { 0xa9, 0xb4, 0x00, 0xaa, 0x00, 0xc0, 0x99, 0x3e }
};

// GLOBAL: LEGORR 0x0076ac80
DirectDraw_Globs directDrawGlobs;

// FUNCTION: LEGORR 0x0047c170
void directdraw_initialise(void* hWnd)
{
    directDrawGlobs.driverCount = 0;
    directDrawGlobs.driverList = NULL;
    directDrawGlobs.deviceCount = 0;
    directDrawGlobs.deviceList = NULL;
    directDrawGlobs.modeCount = 0;
    directDrawGlobs.modeList = NULL;
    directDrawGlobs.hWnd = hWnd;
    directDrawGlobs.lpDirectDraw = NULL;
    directDrawGlobs.zSurf = NULL;
    directDrawGlobs.bSurf = NULL;
    directDrawGlobs.fSurf = NULL;
    directDrawGlobs.lpFrontClipper = NULL;
}

// FUNCTION: LEGORR 0x0047c1c0
int directdraw_enum_drivers(DirectDraw_Driver* list, unsigned int* count)
{
    directDrawGlobs.driverList = list;
    DirectDrawEnumerateA(directdraw_enum_driver_callback, NULL);
    *count = directDrawGlobs.driverCount;
    return TRUE;
}

// FUNCTION: LEGORR 0x0047c1f0
static BOOL WINAPI directdraw_enum_driver_callback(DirectDraw_Guid* lpGUID,
    LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
    directDrawGlobs.driverList[directDrawGlobs.driverCount].flags =
        DIRECTDRAW_FLAG_VALID;
    if (lpGUID) directDrawGlobs.driverList[directDrawGlobs.driverCount].guid = *lpGUID;
    else directDrawGlobs.driverList[directDrawGlobs.driverCount].flags |=
        DIRECTDRAW_FLAG_DRIVER_PRIMARY;
    if (!lpGUID) directDrawGlobs.driverList[directDrawGlobs.driverCount].flags |=
        DIRECTDRAW_FLAG_DRIVER_WINDOWOK;
    sprintf(directDrawGlobs.driverList[directDrawGlobs.driverCount].desc,
            "%s (%s)", lpDriverDescription, lpDriverName);
    directDrawGlobs.driverCount++;
    return TRUE;
}

// FUNCTION: LEGORR 0x0047c2e0
int directdraw_enum_devices(const DirectDraw_Driver* driver,
                            DirectDraw_Device* list, unsigned int* count)
{
    DirectDrawIface1* lpDD;
    DirectDrawIface* lpDD4;
    int res = FALSE;

    directDrawGlobs.deviceCount = 0;
    if (DirectDrawCreate((DirectDraw_Guid*)&driver->guid, &lpDD, NULL) == DD_OK) {
        if (lpDD->lpVtbl->QueryInterface(lpDD, &s_iid_direct_draw4,
                                         (void**)&lpDD4) == DD_OK) {
            Direct3DIface* lpD3D;
            if (lpDD4->lpVtbl->QueryInterface(lpDD4, &s_iid_direct3d3,
                                              (void**)&lpD3D) == DD_OK) {
                directDrawGlobs.deviceList = list;
                lpD3D->lpVtbl->EnumDevices(lpD3D,
                                           directdraw_enum_device_callback,
                                           NULL);
                res = TRUE;
                lpD3D->lpVtbl->Release(lpD3D);
            }
            lpDD4->lpVtbl->Release(lpDD4);
        }
        lpDD->lpVtbl->Release(lpDD);
    }
    *count = directDrawGlobs.deviceCount;
    return res;
}

// FUNCTION: LEGORR 0x0047c380
static HRESULT WINAPI directdraw_enum_device_callback(DirectDraw_Guid* lpGuid,
    LPSTR lpDeviceDescription, LPSTR lpDeviceName,
    Direct3DDeviceDesc* lpHWDesc, Direct3DDeviceDesc* lpHELDesc,
    LPVOID lpContext)
{
    DirectDraw_Device* dev = &directDrawGlobs.deviceList[directDrawGlobs.deviceCount];
    Direct3DDeviceDesc* desc;

    dev->flags = DIRECTDRAW_FLAG_VALID;
    if (lpHWDesc->dcmColorModel) {
        dev->flags = DIRECTDRAW_FLAG_VALID | DIRECTDRAW_FLAG_DEVICE_HARDWARE;
        desc = lpHWDesc;
    } else desc = lpHELDesc;

    if ((desc->dwFlags & D3DDD_COLORMODEL) && desc->dcmColorModel == D3DCOLOR_RGB) {
        dev->flags |= DIRECTDRAW_FLAG_DEVICE_COLOUR;
    }
    if (desc->dwFlags & D3DDD_DEVICERENDERBITDEPTH) {
        if (desc->dwDeviceRenderBitDepth & DDBD_8) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH8;
        if (desc->dwDeviceRenderBitDepth & DDBD_16) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH16;
        if (desc->dwDeviceRenderBitDepth & DDBD_24) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH24;
        if (desc->dwDeviceRenderBitDepth & DDBD_32) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH32;
    }
    if (desc->dwFlags & D3DDD_DEVCAPS) {
        if (desc->dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) dev->flags |= DIRECTDRAW_FLAG_DEVICE_VIDEOTEXTURE;
        else if (desc->dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) dev->flags |= DIRECTDRAW_FLAG_DEVICE_SYSTEMTEXTURE;
    }

    dev->guid = *lpGuid;
    sprintf(dev->desc, "%s (%s)", lpDeviceName, lpDeviceDescription);
    directDrawGlobs.deviceCount++;
    return 1; /* D3DENUMRET_OK */
}

// FUNCTION: LEGORR 0x0047c4b0
int directdraw_enum_modes(const DirectDraw_Driver* driver, int fullScreen,
                          DirectDraw_Mode* list, unsigned int* count)
{
    DirectDrawIface1* lpDD;
    int res = FALSE;
    DirectDraw_Guid* guid;

    directDrawGlobs.modeCount = 0;
    if (driver && (driver->flags & DIRECTDRAW_FLAG_VALID)) {
        if (driver->flags & DIRECTDRAW_FLAG_DRIVER_PRIMARY) guid = NULL;
        else guid = (DirectDraw_Guid*)&driver->guid;
        if (DirectDrawCreate(guid, &lpDD, NULL) == DD_OK) {
            DirectDrawIface* lpDD4;
            if (lpDD->lpVtbl->QueryInterface(lpDD, &s_iid_direct_draw4,
                                             (void**)&lpDD4) == DD_OK) {
                directDrawGlobs.modeList = list;
                lpDD4->lpVtbl->EnumDisplayModes(lpDD4, 0, NULL, &fullScreen,
                                                directdraw_enum_mode_callback);
                res = TRUE;
                lpDD4->lpVtbl->Release(lpDD4);
            }
            lpDD->lpVtbl->Release(lpDD);
        }
    }
    *count = directDrawGlobs.modeCount;
    return res;
}

// FUNCTION: LEGORR 0x0047c550
static HRESULT WINAPI directdraw_enum_mode_callback(
    DirectDrawSurfaceDesc2* lpDDSurfaceDesc, LPVOID lpContext)
{
    DirectDraw_Mode* mode = &directDrawGlobs.modeList[directDrawGlobs.modeCount];

    mode->flags = DIRECTDRAW_FLAG_VALID;
    mode->width = lpDDSurfaceDesc->dwWidth;
    mode->height = lpDDSurfaceDesc->dwHeight;
    mode->bitDepth = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    // STRING: LEGORR 0x004aae6c
    if (*(int*)lpContext) sprintf(mode->desc, "%ix%i (%i bit)", lpDDSurfaceDesc->dwWidth, lpDDSurfaceDesc->dwHeight, lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
    // STRING: LEGORR 0x004aae64
    else sprintf(mode->desc, "%ix%i", lpDDSurfaceDesc->dwWidth, lpDDSurfaceDesc->dwHeight);

    if (!*(int*)lpContext && directDrawGlobs.modeCount &&
        mode->bitDepth != (unsigned int)window_runtime_display_caps_query()) {
        mode->flags &= ~DIRECTDRAW_FLAG_VALID;
        return 1; /* DDENUMRET_OK */
    }
    directDrawGlobs.modeCount++;
    return 1; /* DDENUMRET_OK */
}

// FUNCTION: LEGORR 0x0047c610
int directdraw_setup(int fullScreen, const DirectDraw_Driver* driver,
                     const DirectDraw_Device* device,
                     const DirectDraw_Mode* mode, unsigned int xPos,
                     unsigned int yPos, unsigned int width,
                     unsigned int height)
{
    DirectDrawSurfaceDesc2 ddsd;
    DirectDrawIface1* lpDD;
    DirectDraw_Guid* guid = NULL;
    unsigned int bpp = 16;
    HRESULT r;

    if (driver && !(driver->flags & DIRECTDRAW_FLAG_VALID)) driver = NULL;
    if (device && !(device->flags & DIRECTDRAW_FLAG_VALID)) device = NULL;
    if (mode && !(mode->flags & DIRECTDRAW_FLAG_VALID)) mode = NULL;
    if (mode) {
        width = mode->width;
        height = mode->height;
        bpp = mode->bitDepth;
    }

    directDrawGlobs.width = width;
    directDrawGlobs.height = height;
    directDrawGlobs.fullScreen = fullScreen;
    if (driver) guid = (driver->flags & DIRECTDRAW_FLAG_DRIVER_PRIMARY) ? NULL : (DirectDraw_Guid*)&driver->guid;

    renderer_window_mode_apply(fullScreen, xPos, yPos, width, height);

    if (DirectDrawCreate(guid, &lpDD, NULL) == DD_OK) {
        if (lpDD->lpVtbl->QueryInterface(lpDD, &s_iid_direct_draw4, (void**)&directDrawGlobs.lpDirectDraw) == DD_OK) {
            if (directDrawGlobs.lpDirectDraw->lpVtbl->SetCooperativeLevel(directDrawGlobs.lpDirectDraw, (HWND)directDrawGlobs.hWnd, fullScreen ? DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE : DDSCL_NORMAL) == DD_OK) {
                if (fullScreen) r = directDrawGlobs.lpDirectDraw->lpVtbl->SetDisplayMode(directDrawGlobs.lpDirectDraw, width, height, bpp, 0, 0);
                else r = DD_OK;
                if (r == DD_OK) {
                    memset(&ddsd, 0, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_CAPS;
                    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
                    if (fullScreen) {
                        ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
                        ddsd.dwBackBufferCount = 1;
                        ddsd.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
                    }
                    if (directDrawGlobs.lpDirectDraw->lpVtbl->CreateSurface(directDrawGlobs.lpDirectDraw, &ddsd, &directDrawGlobs.fSurf, NULL) == DD_OK) {
                        if (!fullScreen) {
                            ddsd.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
                            ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
                            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
                            ddsd.dwWidth = width;
                            ddsd.dwHeight = height;
                            r = directDrawGlobs.lpDirectDraw->lpVtbl->CreateSurface(directDrawGlobs.lpDirectDraw, &ddsd, &directDrawGlobs.bSurf, NULL);
                        } else {
                            DirectDrawCaps2 ddscaps;
                            memset(&ddscaps, 0, sizeof(ddscaps));
                            ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
                            r = directDrawGlobs.fSurf->lpVtbl->GetAttachedSurface(directDrawGlobs.fSurf, &ddscaps, &directDrawGlobs.bSurf);
                        }
                        if (r == DD_OK) {
                            if (directdraw_create_clipper(fullScreen, width, height) &&
                                direct3drm_device_initialize((void*)device, (unsigned int)lpDD, directDrawGlobs.bSurf, fullScreen)) {
                                lpDD->lpVtbl->Release(lpDD);
                                if (fullScreen) ShowCursor(FALSE);
                                return TRUE;
                            }
                            directDrawGlobs.bSurf->lpVtbl->Release(directDrawGlobs.bSurf);
                            directDrawGlobs.bSurf = NULL;
                        }
                        directDrawGlobs.fSurf->lpVtbl->Release(directDrawGlobs.fSurf);
                        directDrawGlobs.fSurf = NULL;
                    }
                }
            }
            directDrawGlobs.lpDirectDraw->lpVtbl->Release(directDrawGlobs.lpDirectDraw);
            directDrawGlobs.lpDirectDraw = NULL;
        }
        lpDD->lpVtbl->Release(lpDD);
    }
    return FALSE;
}

// FUNCTION: LEGORR 0x0047c8d0
void directdraw_flip(void)
{
    if (directDrawGlobs.fullScreen) {
        directDrawGlobs.fSurf->lpVtbl->Flip(directDrawGlobs.fSurf, NULL, DDFLIP_WAIT);
        return;
    }
    directdraw_blit_buffers();
}

// FUNCTION: LEGORR 0x0047c8f0
int directdraw_save_bmp(DirectDrawSurfaceIface* surface, const char* fname)
{
    unsigned int bBits = 0;
    DirectDrawBmpHeader header = { 0 };
    DirectDrawSurfaceDesc2 desc;
    File* ofp;
    unsigned char* buffer;
    unsigned int lineSize, size, bytesPerPixel;
    unsigned int rBits = 0, gBits = 0;
    unsigned int x, y, i, offset, out;
    unsigned int value;
    int success = FALSE;
    if ((ofp = file_open(fname, "wb")) != NULL) {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        if (surface->lpVtbl->Lock(surface, NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK) {
            lineSize = desc.dwWidth * 3;
            lineSize = (lineSize / 4) * 4 + ((lineSize % 4) ? 4 : 0);
            size = lineSize * desc.dwHeight;
            if ((buffer = (unsigned char*)malloc(size)) != NULL) {
                memset(buffer, 0, size);
                bytesPerPixel = desc.ddpfPixelFormat.dwRGBBitCount / 8;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwRBitMask >> i) & 1) rBits++;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwGBitMask >> i) & 1) gBits++;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwBBitMask >> i) & 1) bBits++;
                for (y = 0; y < desc.dwHeight; y++) {
                    for (x = 0, offset = 0, out = 0; x < desc.dwWidth; x++) {
                        value = *(unsigned int*)((unsigned char*)desc.lpSurface + y * desc.lPitch + offset);
                        out += 3;
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 1] = (unsigned char)((value >> (gBits + bBits)) << (8 - rBits));
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 2] = (unsigned char)((value >> bBits) << (8 - gBits));
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 3] = (unsigned char)(value << (8 - bBits));
                        offset += bytesPerPixel;
                    }
                }
                success = TRUE;
            }
            surface->lpVtbl->Unlock(surface, NULL);

            header.bfType[0] = 'B';
            header.bfType[1] = 'M';
            header.bfSize = size + sizeof(header);
            header.bfOffBits = sizeof(header);
            header.biSize = 40;
            header.biWidth = desc.dwWidth;
            header.biHeight = desc.dwHeight;
            header.biPlanes = 1;
            header.biBitCount = 24;
            header.biCompression = 0;
            header.biSizeImage = 0;
            header.biXPelsPerMeter = 1;
            header.biYPelsPerMeter = 1;
            header.biClrUsed = 0;
            header.biClrImportant = 0;
            if (success) {
                file_write(&header, sizeof(header), 1, ofp);
                file_write(buffer, size, 1, ofp);
            }
            free(buffer);
        }
        file_close(ofp);
    }
    return success;
}

// FUNCTION: LEGORR 0x0047cc20
void directdraw_return_front_buffer(void)
{
    if (directDrawGlobs.fullScreen) {
        directDrawGlobs.bSurf->lpVtbl->Blt(directDrawGlobs.bSurf, NULL, directDrawGlobs.fSurf, NULL, DDBLT_WAIT, NULL);
    }
}

// FUNCTION: LEGORR 0x0047cc50
void directdraw_blit_buffers(void)
{
    POINT pt;
    RECT dest;
    RECT src;

    dest.right = directDrawGlobs.width;
    pt.y = 0;
    pt.x = 0;
    dest.left = 0;
    dest.top = 0;
    dest.bottom = directDrawGlobs.height;
    ClientToScreen((HWND)directDrawGlobs.hWnd, &pt);
    OffsetRect(&dest, pt.x, pt.y);
    src.left = 0;
    src.top = 0;
    src.right = directDrawGlobs.width;
    src.bottom = directDrawGlobs.height;
    directDrawGlobs.fSurf->lpVtbl->Blt(directDrawGlobs.fSurf, &dest, directDrawGlobs.bSurf, &src, DDBLT_WAIT, NULL);
}

// FUNCTION: LEGORR 0x0047ccf0
void directdraw_shutdown(void)
{
    if (directDrawGlobs.fSurf) directDrawGlobs.fSurf->lpVtbl->Release(directDrawGlobs.fSurf);
    if (directDrawGlobs.lpFrontClipper) directDrawGlobs.lpFrontClipper->lpVtbl->Release(directDrawGlobs.lpFrontClipper);
    if (directDrawGlobs.lpBackClipper) directDrawGlobs.lpBackClipper->lpVtbl->Release(directDrawGlobs.lpBackClipper);
    if (directDrawGlobs.lpDirectDraw) {
        if (directDrawGlobs.fullScreen) directDrawGlobs.lpDirectDraw->lpVtbl->RestoreDisplayMode(directDrawGlobs.lpDirectDraw);
        if (directDrawGlobs.lpDirectDraw) directDrawGlobs.lpDirectDraw->lpVtbl->Release(directDrawGlobs.lpDirectDraw);
    }
}

// FUNCTION: LEGORR 0x0047cd50
void directdraw_adjust_texture_usage(unsigned int* textureUsage)
{
    DirectDrawPixelFormat pf;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    if (lpDevice()->lpVtbl->FindPreferredTextureFormat(lpDevice(), DDBD_8, D3DRMFPTF_PALETTIZED, &pf) != DD_OK) {
        memset(&pf, 0, sizeof(pf));
        pf.dwSize = sizeof(pf);
        if (lpDevice()->lpVtbl->FindPreferredTextureFormat(lpDevice(), DDBD_16, 0, &pf) == DD_OK) {
            *textureUsage *= (pf.dwRGBBitCount / 8);
        }
    }
}

// FUNCTION: LEGORR 0x0047cdd0
int directdraw_get_avail_texture_mem(unsigned int* total, unsigned int* avail)
{
    DirectDrawCaps2 ddsc;

    memset(&ddsc, 0, sizeof(ddsc));
    ddsc.dwCaps = DDSCAPS_TEXTURE;
    *total = 0;
    *avail = 0;
    return directDrawGlobs.lpDirectDraw->lpVtbl->GetAvailableVidMem(directDrawGlobs.lpDirectDraw, &ddsc, total, avail) == DD_OK;
}

// FUNCTION: LEGORR 0x0047ce20
void directdraw_clear(Area2F* window, unsigned int colour)
{
    DirectDrawBltFx ddbltfx;
    RECT rect;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = directdraw_get_colour(directDrawGlobs.bSurf, colour);

    if (window) {
        rect.left = (LONG)window->x;
        rect.top = (LONG)window->y;
        rect.right = (LONG)(window->x + window->width);
        rect.bottom = (LONG)(window->y + window->height);
        directDrawGlobs.bSurf->lpVtbl->Blt(directDrawGlobs.bSurf, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    } else {
        directDrawGlobs.bSurf->lpVtbl->Blt(directDrawGlobs.bSurf, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    }
}

// FUNCTION: LEGORR 0x0047cee0
int directdraw_create_clipper(int fullScreen, unsigned int width, unsigned int height)
{
    HRGN handle;
    DWORD size;
    RGNDATA* region;

    if (directDrawGlobs.lpDirectDraw->lpVtbl->CreateClipper(directDrawGlobs.lpDirectDraw, 0, &directDrawGlobs.lpBackClipper, NULL) == DD_OK) {
        handle = CreateRectRgn(0, 0, width, height);
        size = GetRegionData(handle, 0, NULL);
        region = (RGNDATA*)malloc(size);
        GetRegionData(handle, size, region);
        if (directDrawGlobs.lpBackClipper->lpVtbl->SetClipList(directDrawGlobs.lpBackClipper, region, 0) == DD_OK) {
            free(region);
            if (directDrawGlobs.bSurf->lpVtbl->SetClipper(directDrawGlobs.bSurf, directDrawGlobs.lpBackClipper) == DD_OK) {
                if (!fullScreen) {
                    if (directDrawGlobs.lpDirectDraw->lpVtbl->CreateClipper(directDrawGlobs.lpDirectDraw, 0, &directDrawGlobs.lpFrontClipper, NULL) == DD_OK) {
                        if (directDrawGlobs.lpFrontClipper->lpVtbl->SetHWnd(directDrawGlobs.lpFrontClipper, 0, (HWND)directDrawGlobs.hWnd) == DD_OK) {
                            if (directDrawGlobs.fSurf->lpVtbl->SetClipper(directDrawGlobs.fSurf, directDrawGlobs.lpFrontClipper) == DD_OK) {
                                return TRUE;
                            }
                        }
                    }
                } else return TRUE;
            }
            directDrawGlobs.lpBackClipper->lpVtbl->Release(directDrawGlobs.lpBackClipper);
            directDrawGlobs.lpBackClipper = NULL;
        }
        /* The original frees the region data again after a failure past
         * SetClipList. */
        free(region);
    }
    return FALSE;
}

// FUNCTION: LEGORR 0x0047d000
void directdraw_blt_8_to_16(DirectDrawSurfaceIface* target, DirectDrawSurfaceIface* source, void* palette)
{
    DirectDrawSurfaceDesc2 descSource, descTarget;
    unsigned int x, y, colour, val;
    int i, shift, k, rBits, gBits, bBits;
    unsigned char r, g, b, index;

    memset(&descSource, 0, sizeof(descSource));
    memset(&descTarget, 0, sizeof(descTarget));
    descSource.dwSize = sizeof(descSource);
    descTarget.dwSize = sizeof(descTarget);
    if (source->lpVtbl->Lock(source, NULL, &descSource, DDLOCK_WAIT, NULL) == DD_OK) {
        if (descSource.ddpfPixelFormat.dwRGBBitCount == 8 &&
            target->lpVtbl->Lock(target, NULL, &descTarget, DDLOCK_WAIT, NULL) == DD_OK) {
            if (descTarget.ddpfPixelFormat.dwRGBBitCount == 16 &&
                descTarget.dwWidth == descSource.dwWidth &&
                descTarget.dwHeight == descSource.dwHeight) {
                for (i = 0, rBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwRBitMask >> i) & 1) rBits++;
                for (i = 0, gBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwGBitMask >> i) & 1) gBits++;
                for (i = 0, bBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwBBitMask >> i) & 1) bBits++;
                for (y = 0; y < descSource.dwHeight; y++) {
                    for (x = 0; x < descSource.dwWidth; x += 4) {
                        val = *(unsigned int*)((unsigned char*)descSource.lpSurface + y * descSource.lPitch + x);
                        for (shift = 24, k = 3; shift >= 0; shift -= 8, k--) {
                            index = (unsigned char)(val >> shift);
                            r = ((DirectDrawPaletteEntry*)palette)[index].peRed >> (8 - rBits);
                            g = ((DirectDrawPaletteEntry*)palette)[index].peGreen >> (8 - gBits);
                            b = ((DirectDrawPaletteEntry*)palette)[index].peBlue >> (8 - bBits);
                            if (k & 1) {
                                colour = (r << (gBits + bBits) | g << bBits | b) << 16;
                            } else {
                                colour |= r << (gBits + bBits) | g << bBits | b;
                                *(unsigned int*)((unsigned char*)descTarget.lpSurface + y * descTarget.lPitch + (x + ((k >> 1) & 1) * 2) * 2) = colour;
                            }
                        }
                    }
                }
            }
            target->lpVtbl->Unlock(target, NULL);
        }
        source->lpVtbl->Unlock(source, NULL);
    }
}

// FUNCTION: LEGORR 0x0047d2d0
unsigned int directdraw_get_colour(DirectDrawSurfaceIface* surf, unsigned int colour)
{
    DirectDrawPaletteEntry entries[256];
    DirectDrawPixelFormat pf;
    DirectDrawPaletteIface* pal;
    unsigned int r = (colour >> 16) & 0xff, g = (colour >> 8) & 0xff, b = colour & 0xff;
    unsigned int rBits, gBits, bBits, i;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    surf->lpVtbl->GetPixelFormat(surf, &pf);
    if (pf.dwFlags & DDPF_RGB) {
        rBits = directdraw_count_mask_bits(pf.dwRBitMask);
        gBits = directdraw_count_mask_bits(pf.dwGBitMask);
        bBits = directdraw_count_mask_bits(pf.dwBBitMask);
        return ((r >> (8 - rBits)) << (gBits + bBits)) | ((g >> (8 - gBits)) << bBits) | (b >> (8 - bBits));
    }
    surf->lpVtbl->GetPalette(surf, &pal);
    pal->lpVtbl->GetEntries(pal, 0, 0, 256, entries);
    for (i = 0; i < 256; i++) {
        if (entries[i].peRed == r && entries[i].peGreen == g && entries[i].peBlue == b) return i;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047d3f0
unsigned int directdraw_count_mask_bits(unsigned int mask)
{
    unsigned int count = 0;
    while (mask) {
        mask &= mask - 1;
        count++;
    }
    return count;
}

#endif /* !LEGORRR_BACKEND_MINIWIN */
