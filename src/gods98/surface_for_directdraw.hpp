// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what directdraw uses from surface.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SURFACE_FOR_DIRECTDRAW_HPP
#define LRR_GODS98_SURFACE_FOR_DIRECTDRAW_HPP

/* The WinAPI build intentionally avoids miniwin's shortened COM interfaces.
 * The reference requests IDirectDraw4/IDirectDrawSurface4 and passes the
 * 124-byte DDSURFACEDESC2 layout. Declare only the ABI pieces used here. */
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
typedef struct SurfaceDdrawColorKey {
    DWORD low;
    DWORD high;
} SurfaceDdrawColorKey;

typedef struct SurfaceDdrawPixelFormat {
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD rgb_bits;
    DWORD red_mask;
    DWORD green_mask;
    DWORD blue_mask;
    DWORD alpha_mask;
} SurfaceDdrawPixelFormat;

typedef struct SurfaceDdrawPaletteEntry {
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE flags;
} SurfaceDdrawPaletteEntry;
typedef char SurfaceDdrawPaletteEntryMustBe4Bytes[
    sizeof(SurfaceDdrawPaletteEntry) == 4u ? 1 : -1];

typedef struct SurfaceDdrawCaps2 {
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
} SurfaceDdrawCaps2;

typedef struct SurfaceDdrawDesc2 {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    union { LONG pitch; DWORD linear_size; };
    DWORD backbuffer_count;
    union { DWORD mipmap_count; DWORD zbuffer_depth; DWORD refresh_rate; };
    DWORD alpha_depth;
    DWORD reserved;
    void* surface;
    SurfaceDdrawColorKey keys[4];
    SurfaceDdrawPixelFormat pixel_format;
    SurfaceDdrawCaps2 caps;
    DWORD texture_stage;
} SurfaceDdrawDesc2;

typedef char SurfaceDdrawDesc2MustBe124Bytes[
    sizeof(SurfaceDdrawDesc2) == 0x7cu ? 1 : -1];
typedef char SurfaceDdrawPixelFormatMustBe32Bytes[
    sizeof(SurfaceDdrawPixelFormat) == 0x20u ? 1 : -1];
#endif

#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
typedef HRESULT (WINAPI *SurfaceDdrawCreateFn)(GUID*, void**, void*);
typedef HRESULT (WINAPI *SurfaceQueryInterfaceFn)(void*, REFIID, void**);
typedef ULONG (WINAPI *SurfaceReleaseFn)(void*);
typedef HRESULT (WINAPI *SurfaceSetCooperativeLevelFn)(void*, HWND, DWORD);
typedef HRESULT (WINAPI *SurfaceSetDisplayModeFn)(void*, DWORD, DWORD,
                                                   DWORD, DWORD, DWORD);
typedef HRESULT (WINAPI *SurfaceCreateSurfaceFn)(void*, SurfaceDdrawDesc2*,
                                                 void**, void*);
typedef HRESULT (WINAPI *SurfaceCreateClipperFn)(void*, DWORD, void**, void*);
typedef HRESULT (WINAPI *SurfaceCreatePaletteFn)(void*, DWORD,
    SurfaceDdrawPaletteEntry*, void**, void*);
typedef HRESULT (WINAPI *SurfaceGetAttachedSurfaceFn)(void*,
    SurfaceDdrawCaps2*, void**);
typedef HRESULT (WINAPI *SurfaceSetClipperFn)(void*, void*);
typedef HRESULT (WINAPI *ClipperSetClipListFn)(void*, const RGNDATA*, DWORD);
typedef HRESULT (WINAPI *SurfaceLockFn)(void*, RECT*, SurfaceDdrawDesc2*,
                                        DWORD, HANDLE);
typedef HRESULT (WINAPI *SurfaceUnlockFn)(void*, RECT*);
typedef HRESULT (WINAPI *SurfaceSetPaletteFn)(void*, void*);
typedef HRESULT (WINAPI *SurfacePaletteSetEntriesFn)(void*, DWORD, DWORD,
    DWORD, SurfaceDdrawPaletteEntry*);
typedef HRESULT (WINAPI *SurfaceBltFn)(void*, RECT*, void*, RECT*, DWORD,
                                       void*);
typedef HRESULT (WINAPI *SurfaceGetDescriptionFn)(void*,
                                                   SurfaceDdrawDesc2*);
typedef HRESULT (WINAPI *SurfaceGetPixelFormatFn)(void*,
                                                  SurfaceDdrawPixelFormat*);
#endif

#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
typedef struct WinapiRendererState {
    void* device;
    void* primary;
    void* back_surface;
    void* clipper;
    void* rm;
    void* rm_device;
    void* immediate_device;
    void* staging_surface;
    HMODULE rm_module;
    HWND window;
    int fullscreen;
    int width;
    int height;
} WinapiRendererState;
#endif

extern uint32_t s_renderer_window_mode_flags;
extern DirectDrawEnumerationHook s_directdraw_enumeration_hook;
extern void* s_directdraw_enumeration_context;
extern ProjectionOverlayBlitHook s_projection_overlay_blit_hook;
extern void* s_projection_overlay_blit_context;
extern DirectDrawClipperRegionOps s_directdraw_clipper_region_ops;
extern DirectDrawDeviceCreateOps s_directdraw_device_create_ops;
extern DirectDrawRendererInitOps s_directdraw_renderer_init_ops;
#if !defined(_WIN32)
extern void* s_surface_description_interface;
#endif
extern Direct3DTextureMemoryCapsQueryFn s_texture_memory_caps_query;
extern void* s_texture_memory_caps_context;
extern uint32_t s_texture_memory_last_value;
extern int s_direct3drm_device_mode;
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
extern const GUID s_iid_direct_draw4;
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
void** surface_com_vtable(void* object);
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
ULONG surface_com_release(void* object);
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
int surface_ddraw_api_get(SurfaceDdrawCreateFn* create);
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
extern WinapiRendererState s_winapi_renderer_state;
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
extern uint32_t s_renderer_fullscreen;
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
extern uint32_t s_renderer_surface_width;
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
extern uint32_t s_renderer_surface_height;
#endif
#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
void winapi_renderer_state_release(WinapiRendererState* state);
#endif

#endif /* LRR_GODS98_SURFACE_FOR_DIRECTDRAW_HPP */
