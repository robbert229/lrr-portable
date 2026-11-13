// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail of G98-010 (finding 596): the port's display layer, which the
 * native and Windows renderers run. DirectDraw.c itself is directdraw.c,
 * rebuilt in original shape; these functions keep the port's names and
 * contracts (hook tables, the port's renderer state) and carry no
 * annotations. #540 relocated them from the unit verbatim
 * (docs/coverage/relocations/G98-010.csv).
 *
 * Two bridges to the unit, both port-only:
 * - Windows builds: the port renderer publishes the display it commits
 *   (window, DirectDraw object, buffers, mode and Main.c's device) into
 *   the unit's directDrawGlobs, so the original-shape readers of that state
 *   (draw.c, aitask.c and the unit itself) see the same display.
 * - Native builds have no DirectDraw: the unit is empty there, and the
 *   entry points that converted units call get native bodies here, with
 *   the behaviour of the adapters they replace. */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "platform/platform.h"
#include "surface.hpp"
#include "model.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "engine.hpp"
#include "heap.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "frameloop.hpp"
#include "scene_resources.hpp"
#include "surface_for_images_unit.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include "string.hpp"
#include "game.hpp"
#include "game/entity.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "savegame.hpp"
#include "options.hpp"
#include "util.hpp"
#include "savegame_shared.hpp"
#include <time.h>
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/effects.hpp"
#include "game/objective.hpp"
#include "game/port_loop_ports.hpp"
#include "game/render.hpp"
#include "game/select.hpp"
#include "game/tutorial_hud_ports.hpp"
#include "game/tutorial_world_arrows.hpp"
#include "game/scene_render.hpp"
#include "game/ui_action_dispatch.hpp"
#include "game/upgrades.hpp"
#include "audio.hpp"
#include "input.hpp"
#include "movie.hpp"
#include "media.hpp"
#include "flic.hpp"
#include "message.hpp"
#include "sound.hpp"
#include "tickers.hpp"
#include "simtick.hpp"
#include "camera.hpp"
#include "surface_for_directdraw.hpp"
#include "directdraw.h"
#include "directdraw_imports.h"

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static int directdraw_renderer_init_winapi(
    const DirectDrawRendererInitParams* params, const void* guid,
    int width, int height, int bits_per_pixel);
static int winapi_renderer_active(void);
#endif

#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
/* Publish the committed display into DirectDraw.c's state (see above). */
static void directdraw_port_publish(void* window, void* direct_draw,
                                    void* front, void* back, int fullscreen,
                                    int width, int height, void* device)
{
    directDrawGlobs.hWnd = window;
    directDrawGlobs.lpDirectDraw = (struct DirectDrawIface*)direct_draw;
    directDrawGlobs.fSurf = (struct DirectDrawSurfaceIface*)front;
    directDrawGlobs.bSurf = (struct DirectDrawSurfaceIface*)back;
    directDrawGlobs.fullScreen = fullscreen != 0;
    directDrawGlobs.width = (unsigned int)width;
    directDrawGlobs.height = (unsigned int)height;
    s_main_device = device;
}
#endif

#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static int winapi_renderer_clipper_attach(void* device, void* surface,
    HWND window, void** clipper_out)
{
    SurfaceCreateClipperFn create_clipper;
    typedef HRESULT (WINAPI *ClipperSetHwndFn)(void*, DWORD, HWND);
    ClipperSetHwndFn set_hwnd;
    SurfaceSetClipperFn set_clipper;
    void** vtable;
    void* clipper = NULL;
    int success = 0;

    if (clipper_out != NULL) *clipper_out = NULL;
    if (device == NULL || surface == NULL || clipper_out == NULL ||
        window == NULL) return 0;
    vtable = surface_com_vtable(device);
    create_clipper = vtable != NULL && vtable[4] != NULL
        ? (SurfaceCreateClipperFn)vtable[4] : NULL;
    if (create_clipper == NULL ||
        create_clipper(device, 0u, &clipper, NULL) != 0 || clipper == NULL)
        goto cleanup;

    vtable = surface_com_vtable(clipper);
    set_hwnd = vtable != NULL && vtable[8] != NULL
        ? (ClipperSetHwndFn)vtable[8] : NULL;
    if (set_hwnd == NULL || set_hwnd(clipper, 0u, window) != 0) goto cleanup;
    vtable = surface_com_vtable(surface);
    set_clipper = vtable != NULL && vtable[28] != NULL
        ? (SurfaceSetClipperFn)vtable[28] : NULL;
    if (set_clipper == NULL || set_clipper(surface, clipper) != 0)
        goto cleanup;
    *clipper_out = clipper;
    clipper = NULL;
    success = 1;

cleanup:
    if (clipper != NULL) surface_com_release(clipper);
    return success;
}

static int winapi_rm_device_create(void* rm1, void* device, void* back_surface,
    const GUID* device_guid, void** rm_out, void** device_out)
{
    typedef HRESULT (WINAPI *CreateDeviceFn)(void*, const GUID*, void*,
                                              void*, DWORD, void**);
    static const GUID iid_rm3 = { 0x4516ec83, 0x8f20, 0x11d0,
        { 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3 } };
    static const GUID iid_dd1 = { 0x6c14db80, 0xa733, 0x11ce,
        { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };
    static const GUID iid_surface1 = { 0x6c14db81, 0xa733, 0x11ce,
        { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };
    void* dd1 = NULL;
    void* surface1 = NULL;
    int ok = 0;
    *rm_out = NULL;
    *device_out = NULL;
    if (device_guid != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(rm1)[0])(
            rm1, iid_rm3, rm_out) == 0 && *rm_out != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(device)[0])(
            device, iid_dd1, &dd1) == 0 && dd1 != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(back_surface)[0])(
            back_surface, iid_surface1, &surface1) == 0 && surface1 != NULL &&
        ((CreateDeviceFn)surface_com_vtable(*rm_out)[15])(
            *rm_out, device_guid, dd1, surface1, 0u, device_out) == 0 &&
        *device_out != NULL) ok = 1;
    if (surface1 != NULL) surface_com_release(surface1);
    if (dd1 != NULL) surface_com_release(dd1);
    return ok;
}

static int directdraw_renderer_init_winapi(
    const DirectDrawRendererInitParams* params, const void* guid,
    int width, int height, int bits_per_pixel)
{
    SurfaceDdrawCreateFn create;
    SurfaceQueryInterfaceFn query;
    SurfaceSetCooperativeLevelFn set_cooperative_level;
    SurfaceSetDisplayModeFn set_display_mode;
    SurfaceCreateSurfaceFn create_surface;
    SurfaceGetAttachedSurfaceFn get_attached_surface;
    void* direct_draw = NULL;
    void* device = NULL;
    void* primary = NULL;
    void* back_surface = NULL;
    void* clipper = NULL;
    void* rm = NULL;
    void* rm_device = NULL;
    void* immediate_device = NULL;
    HMODULE rm_module = NULL;
    int mode_set = 0;
    void** vtable;
    SurfaceDdrawDesc2 desc;
    SurfaceDdrawCaps2 back_caps;
    HWND window;
    int success = 0;

    if (params == NULL || width <= 0 || height <= 0 ||
        !surface_ddraw_api_get(&create)) return 0;
    window = params->window != NULL ? (HWND)params->window : GetActiveWindow();
    if (window == NULL) return 0;
    if (create((GUID*)guid, &direct_draw, NULL) != 0 || direct_draw == NULL)
        goto cleanup;
    vtable = surface_com_vtable(direct_draw);
    query = vtable != NULL && vtable[0] != NULL
        ? (SurfaceQueryInterfaceFn)vtable[0] : NULL;
    if (query == NULL || query(direct_draw, s_iid_direct_draw4, &device) != 0 ||
        device == NULL) goto cleanup;
    surface_com_release(direct_draw);
    direct_draw = NULL;

    vtable = surface_com_vtable(device);
    set_cooperative_level = vtable != NULL && vtable[20] != NULL
        ? (SurfaceSetCooperativeLevelFn)vtable[20] : NULL;
    if (set_cooperative_level == NULL ||
        set_cooperative_level(device, window,
            params->fullscreen != 0 ? 0x11u : 8u) != 0) goto cleanup;
    if (params->fullscreen != 0) {
        set_display_mode = vtable != NULL && vtable[21] != NULL
            ? (SurfaceSetDisplayModeFn)vtable[21] : NULL;
        if (set_display_mode == NULL || set_display_mode(device,
                (DWORD)width, (DWORD)height, (DWORD)bits_per_pixel,
                0u, 0u) != 0) goto cleanup;
        mode_set = 1;
    }

    memset(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);
    if (params->fullscreen != 0) {
        desc.flags = 0x21u;
        desc.backbuffer_count = 1u;
        desc.caps.caps = 0x2218u;
    }
    else {
        desc.flags = 1u;
        desc.caps.caps = 0x2200u;
    }
    create_surface = vtable != NULL && vtable[6] != NULL
        ? (SurfaceCreateSurfaceFn)vtable[6] : NULL;
    if (create_surface == NULL ||
        create_surface(device, &desc, &primary, NULL) != 0 ||
        primary == NULL) goto cleanup;

    if (params->fullscreen != 0) {
        memset(&back_caps, 0, sizeof(back_caps));
        back_caps.caps = 4u;
        vtable = surface_com_vtable(primary);
        get_attached_surface = vtable != NULL && vtable[12] != NULL
            ? (SurfaceGetAttachedSurfaceFn)vtable[12] : NULL;
        if (get_attached_surface == NULL ||
            get_attached_surface(primary, &back_caps, &back_surface) != 0 ||
            back_surface == NULL) goto cleanup;
    }
    else {
        memset(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        desc.flags = 7u;
        desc.height = (DWORD)height;
        desc.width = (DWORD)width;
        desc.caps.caps = 0x2040u;
        vtable = surface_com_vtable(device);
        create_surface = vtable != NULL && vtable[6] != NULL
            ? (SurfaceCreateSurfaceFn)vtable[6] : NULL;
        if (create_surface == NULL ||
            create_surface(device, &desc, &back_surface, NULL) != 0 ||
            back_surface == NULL) goto cleanup;
    }

    if (params->fullscreen == 0 &&
        !winapi_renderer_clipper_attach(device, primary, window,
                                        &clipper)) goto cleanup;

    /* RM3 consumes the selected Direct3D device GUID, independently of the
     * DirectDraw adapter GUID used to create the owning display. */
    {
        typedef HRESULT (WINAPI *CreateRMFn)(void**);
        char path[MAX_PATH];
        DWORD length = GetFullPathNameA("D3DRM.DLL", sizeof(path), path, NULL);
        void* rm1 = NULL;
        CreateRMFn create_rm;
        int ok = 0;
        if (params->device_guid != NULL && length != 0 && length < sizeof(path)) {
            rm_module = LoadLibraryA(path);
            create_rm = rm_module != NULL ? (CreateRMFn)GetProcAddress(
                rm_module, "Direct3DRMCreate") : NULL;
            if (create_rm != NULL && create_rm(&rm1) == 0 && rm1 != NULL &&
                winapi_rm_device_create(rm1, device, back_surface,
                    (const GUID*)params->device_guid, &rm, &rm_device)) ok = 1;
        }
        if (rm1 != NULL) surface_com_release(rm1);
        if (!ok) goto cleanup;
    }

    /* The UserVisual manager needs the real immediate device belonging to
     * this selected retained-mode device, as4781d0 does. Never substitute
     * the DirectDraw device or the RM factory for that interface. */
    if (!scene_winapi_device3_acquire(rm_device, &immediate_device))
        goto cleanup;

    if (s_directdraw_renderer_init_ops.renderer_state_commit != NULL) {
        s_directdraw_renderer_init_ops.renderer_state_commit(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen);
    }
    winapi_renderer_state_release(&s_winapi_renderer_state);
    s_winapi_renderer_state.device = device;
    s_winapi_renderer_state.primary = primary;
    s_winapi_renderer_state.back_surface = back_surface;
    s_winapi_renderer_state.clipper = clipper;
    s_winapi_renderer_state.rm = rm;
    s_winapi_renderer_state.rm_device = rm_device;
    s_winapi_renderer_state.immediate_device = immediate_device;
    s_winapi_renderer_state.rm_module = rm_module;
    s_winapi_renderer_state.window = window;
    s_winapi_renderer_state.fullscreen = params->fullscreen != 0;
    s_winapi_renderer_state.width = width;
    s_winapi_renderer_state.height = height;
    s_renderer_window_handle = window;
    s_renderer_primary_surface = primary;
    render_surface_secondary_set((uint32_t)back_surface);
    s_renderer_fullscreen = params->fullscreen != 0 ? 1u : 0u;
    s_renderer_surface_width = (uint32_t)width;
    s_renderer_surface_height = (uint32_t)height;
    screen_bounded_surface_device_set(rm, rm_device);
    directdraw_port_publish(window, device, primary, back_surface,
                            params->fullscreen, width, height, rm_device);
    if ((params->device_flags & 0x4000u) != 0u)
        s_renderer_window_mode_flags |= 4u;
    s_direct3drm_device_mode =
        (s_renderer_window_mode_flags & 0x80000u) == 0u &&
        (params->device_flags & 0x2000u) != 0u ? 2 : 1;
    device = NULL;
    primary = NULL;
    back_surface = NULL;
    clipper = NULL;
    rm = NULL; rm_device = NULL; immediate_device = NULL;
    rm_module = NULL; mode_set = 0;
    if (params->fullscreen != 0) (void)ShowCursor(FALSE);
    success = 1;

cleanup:
    if (immediate_device != NULL) surface_com_release(immediate_device);
    if (rm_device != NULL) surface_com_release(rm_device);
    if (rm != NULL) surface_com_release(rm);
    if (rm_module != NULL) FreeLibrary(rm_module);
    if (clipper != NULL) surface_com_release(clipper);
    if (back_surface != NULL) surface_com_release(back_surface);
    if (primary != NULL) surface_com_release(primary);
    if (device != NULL && mode_set) {
        typedef HRESULT (WINAPI *RestoreFn)(void*);
        void** vt = surface_com_vtable(device);
        if (vt != NULL && vt[19] != NULL) ((RestoreFn)vt[19])(device);
    }
    if (device != NULL) surface_com_release(device);
    if (direct_draw != NULL) surface_com_release(direct_draw);
    return success;
}
#endif

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int directdraw_device_create_internal( const void* guid, const void* adapter, int windowed, void** result_out);


void window_state_initialize_retail(void* window)
{
    s_renderer_window_state_b0 = 0u;
    s_renderer_window_state_9c = 0u;
    s_renderer_window_state_b4 = 0u;
    s_renderer_window_state_a4 = 0u;
    s_renderer_window_state_b8 = 0u;
    s_renderer_window_state_ac = 0u;
    s_renderer_window_handle = window;
    s_renderer_global_dword = 0u;
    s_renderer_window_state_90 = 0u;
    s_render_surface_secondary = 0u;
    s_renderer_primary_surface = NULL;
    s_renderer_window_state_94 = 0u;
#if !defined(LEGORRR_BACKEND_MINIWIN)
    directdraw_initialise(window);
#endif
}

/* DirectDrawEnumerateAndStoreCallbackResult (FUN_0047c1c0): enumerate using
 * the requested adapter identifier and return the callback's stored result. */

int directdraw_enumerate_and_store_result(const void* guid, void** result_out)
{
    void* result = NULL;
    if (s_directdraw_enumeration_hook != NULL) {
        s_directdraw_enumeration_hook(s_directdraw_enumeration_context,
                                      guid, &result);
    }
    if (result_out != NULL) *result_out = result;
    return 1;
}

/* DirectDrawDeviceCreate (FUN_0047c2e0): create a device from its optional
 * GUID, query the required interface and run the display-mode enumeration. */

int directdraw_device_create(const void* guid, void** result_out)
{
    if (result_out != NULL) *result_out = NULL;
    return directdraw_device_create_internal(guid, NULL, 0, result_out);
}

/* DirectDrawEnumeratedDeviceCreate (FUN_0047c4b0): require the adapter's
 * enumerated flag, choose its GUID only when flag 0x10 is set, and dispatch
 * mode enumeration with the requested windowed/fullscreen policy. */

int directdraw_enumerated_device_create(const void* adapter,
                                        uint32_t adapter_flags,
                                        int windowed, void** result_out)
{
    const void* guid;
    if (result_out != NULL) *result_out = NULL;
    if (adapter == NULL || (adapter_flags & 1u) == 0u) return 0;
    guid = ((adapter_flags & 0x10u) != 0u) ? adapter : NULL;
    return directdraw_device_create_internal(guid, adapter, windowed,
                                             result_out);
}

/* DirectDrawRendererInit (FUN_0047c610): normalize the adapter and mode
 * selection, configure the window/device, create primary and back surfaces,
 * attach the clipper, initialize Direct3D, and commit the resulting state. */

int directdraw_renderer_init(const DirectDrawRendererInitParams* params)
{
    const void* guid = NULL;
    int width;
    int height;
    int bits_per_pixel;
    void* device = NULL;
    void* primary = NULL;
    void* back_surface = NULL;
    int success = 0;
    if (params == NULL) return 0;
    width = params->width;
    height = params->height;
    bits_per_pixel = params->mode_bits_per_pixel > 0
        ? params->mode_bits_per_pixel : 16;
    if ((params->adapter_flags & 1u) != 0u) {
        guid = params->adapter_guid;
    }
    if (params->mode_width > 0) width = params->mode_width;
    if (params->mode_height > 0) height = params->mode_height;
    if (width <= 0 || height <= 0) return 0;

    if (s_directdraw_renderer_init_ops.device_create == NULL ||
        s_directdraw_renderer_init_ops.cooperative_level_set == NULL ||
        s_directdraw_renderer_init_ops.surfaces_create == NULL ||
        s_directdraw_renderer_init_ops.clipper_setup == NULL ||
        s_directdraw_renderer_init_ops.direct3d_initialize == NULL) {
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        /* Do not disturb an already committed display while attempting a
         * replacement: DirectDraw changes the display mode process-wide. */
        if (winapi_renderer_active()) return 0;
        if (!directdraw_renderer_init_winapi(params, guid, width, height,
                                            bits_per_pixel)) return 0;
        renderer_window_mode_apply(params->fullscreen, params->window_x,
                                   params->window_y, width, height);
        return 1;
#else
        return 0;
#endif
    }
    s_renderer_window_handle = params->window;
    renderer_window_mode_apply(params->fullscreen, params->window_x,
                               params->window_y, width, height);
    if (!s_directdraw_renderer_init_ops.device_create(
            s_directdraw_renderer_init_ops.context, guid, &device) ||
        device == NULL) goto cleanup;
    if (!s_directdraw_renderer_init_ops.cooperative_level_set(
            s_directdraw_renderer_init_ops.context, device,
            params->fullscreen,
            params->fullscreen != 0 ? 0x11u : 8u)) goto cleanup;
    if (params->fullscreen != 0) {
        if (s_directdraw_renderer_init_ops.display_mode_set == NULL ||
            !s_directdraw_renderer_init_ops.display_mode_set(
                s_directdraw_renderer_init_ops.context, device, width,
                height, bits_per_pixel)) goto cleanup;
    }
    if (!s_directdraw_renderer_init_ops.surfaces_create(
            s_directdraw_renderer_init_ops.context, device,
            params->fullscreen, width, height, bits_per_pixel,
            &primary, &back_surface) || primary == NULL ||
        back_surface == NULL) goto cleanup;
    if (!s_directdraw_renderer_init_ops.clipper_setup(
            s_directdraw_renderer_init_ops.context, width, height)) {
        goto cleanup;
    }
    if (!s_directdraw_renderer_init_ops.direct3d_initialize(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen)) goto cleanup;
    if (s_directdraw_renderer_init_ops.renderer_state_commit != NULL) {
        s_directdraw_renderer_init_ops.renderer_state_commit(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen);
    }
    success = 1;

cleanup:
    if (!success && s_directdraw_renderer_init_ops.release != NULL) {
        if (back_surface != NULL) {
            s_directdraw_renderer_init_ops.release(
                s_directdraw_renderer_init_ops.context, back_surface);
        }
        if (primary != NULL) {
            s_directdraw_renderer_init_ops.release(
                s_directdraw_renderer_init_ops.context, primary);
        }
    }
    if (device != NULL && s_directdraw_renderer_init_ops.release != NULL) {
        s_directdraw_renderer_init_ops.release(
            s_directdraw_renderer_init_ops.context, device);
    }
    return success;
}


/* Saves\<slot>.dat — the `.dat` slot thumbnail (FUN_00413b80's save-confirm
 * dump via FUN_0047e440). FUN_0047c8f0's layout, field for field: `BM`,
 * file size = data + 54, reserved 0, data offset 54, DIB header 40, size,
 * positive height (bottom-up), planes 1, 24 bpp, BI_RGB, image size 0,
 * XPel/YPel 1, palette counts 0; pixels BGR, rows padded to four bytes.
 * The port's surface is B,G,R,A in memory (see fe_blit_glyph), so the
 * component map is a straight byte copy. */

int save_write_thumbnail(GameContext* ctx, int slot, const void* fb,
                         int pitch, int w, int h)
{
    if (ctx == NULL || fb == NULL || slot < 0 || slot >= SAVE_SLOTS
        || w <= 0 || h <= 0 || w > 640 || pitch < w * 4) {
        return 0;
    }
    const int stride = (w * 3 + 3) & ~3;
    const uint32_t data_off = 54u;
    const uint32_t file_size = data_off + (uint32_t)stride * (uint32_t)h;

    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B';
    hdr[1] = 'M';
    memcpy(hdr + 2, &file_size, 4);
    const uint32_t dib = 40u;
    const uint32_t off32 = data_off;
    const int32_t w32 = w, h32 = h;
    const uint32_t zero = 0;
    const uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 10, &off32, 4);
    memcpy(hdr + 14, &dib, 4);
    memcpy(hdr + 18, &w32, 4);
    memcpy(hdr + 22, &h32, 4);
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    memcpy(hdr + 30, &zero, 4);            /* BI_RGB */
    memcpy(hdr + 34, &zero, 4);            /* image size 0 */
    const uint32_t ppm = 1;
    memcpy(hdr + 38, &ppm, 4);
    memcpy(hdr + 42, &ppm, 4);

    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "dat");
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    int ok = fwrite(hdr, 1, sizeof(hdr), fp) == sizeof(hdr);
    for (int y = 0; y < h && ok; ++y) {
        /* Bottom-up rows: the surface's row 0 (screen top) lands in the
         * file's last row (FUN_0047c8f0's `h - 1 - y` write offset). */
        const uint8_t* src = (const uint8_t*)fb
                           + (size_t)(h - 1 - y) * (size_t)pitch;
        uint8_t line[1920];               /* (640 * 3) row cap */
        for (int x = 0; x < w; ++x) {
            memcpy(line + x * 3, src + (size_t)x * 4, 3);
        }
        if (stride > w * 3) {
            memset(line + w * 3, 0, (size_t)stride - (size_t)w * 3);
        }
        ok = fwrite(line, 1, (size_t)stride, fp) == (size_t)stride;
    }
    fclose(fp);
    return ok;
}

/* DirectDrawObjectsRelease (FUN_0047ccf0): release the primary surface and
 * clipper interfaces, restore display mode when the device was exclusive,
 * then release the DirectDraw device itself. */

void directdraw_objects_release(DirectDrawObjectSet* objects,
                                const DirectDrawObjectReleaseOps* ops)
{
    if (objects == NULL) return;
    if (ops != NULL && ops->release != NULL) {
        if (objects->primary_surface != NULL)
            ops->release(ops->context, objects->primary_surface);
        if (objects->primary_clipper != NULL)
            ops->release(ops->context, objects->primary_clipper);
        if (objects->windowed_clipper != NULL)
            ops->release(ops->context, objects->windowed_clipper);
    }
    objects->primary_surface = NULL;
    objects->primary_clipper = NULL;
    objects->windowed_clipper = NULL;
    if (objects->device != NULL) {
        if (objects->restore_display_mode != 0 && ops != NULL &&
            ops->restore_display_mode != NULL) {
            ops->restore_display_mode(ops->context, objects->device);
        }
        if (ops != NULL && ops->release != NULL)
            ops->release(ops->context, objects->device);
    }
    objects->device = NULL;
    objects->restore_display_mode = 0;
}
/* QueryTextureMem (FUN_0047cd50): try the preferred texture-memory caps
 * query, then the fallback capability path, and retain the reported amount. */

void query_texture_memory(void)
{
    uint32_t memory = 0u;
    if (s_texture_memory_caps_query == NULL) return;
    if (s_texture_memory_caps_query(s_texture_memory_caps_context,
                                    0x800u, &memory) != 0) {
        memory = 0u;
        if (s_texture_memory_caps_query(s_texture_memory_caps_context,
                                        0x400u, &memory) != 0) return;
    }
    s_texture_memory_last_value = memory;
}

/* SurfaceDescriptionGet (FUN_0047cdd0): pass a 0x1000 description request
 * through slot 0x5c of the renderer's DirectDraw wrapper. */

int surface_description_get(int* width_out, int* height_out)
{
    typedef long (SURFACE_COM_CALL *SurfaceDescriptionProc)(
        void*, uint32_t*, int*, int*);
    uint32_t description[4];
    void* interface_pointer;
    void** vtable;
    SurfaceDescriptionProc get_description;
    description[0] = 0x1000u;
    description[1] = 0u;
    description[2] = 0u;
    description[3] = 0u;
    *width_out = 0;
    *height_out = 0;
#if defined(_WIN32)
    interface_pointer = (void*)s_renderer_global_dword;
#else
    interface_pointer = s_surface_description_interface;
    if (interface_pointer == NULL) return 0;
#endif
    vtable = *(void***)interface_pointer;
    get_description = (SurfaceDescriptionProc)vtable[0x5c / 4];
    return get_description(interface_pointer, description,
                           width_out, height_out) == 0;
}
/* ProjectionOverlayDrawSubmit (FUN_0047ce20): encode the requested fill
 * color for the target surface and submit a color-fill blit, optionally
 * constrained to the supplied rectangle. */

int projection_overlay_draw_submit(int has_rectangle,
                                   const int rectangle[4],
                                   const SurfacePixelFormat* format,
                                   uint32_t color_rgb)
{
    uint32_t fill_color;
    if (s_projection_overlay_blit_hook == NULL ||
        (has_rectangle != 0 && rectangle == NULL)) return 0;
    fill_color = surface_pixel_value_encode(format, color_rgb);
    return s_projection_overlay_blit_hook(
        s_projection_overlay_blit_context,
        has_rectangle != 0 ? rectangle : NULL,
        fill_color, 0x01000400u);
}

/* DirectDrawClipperSetRegion (FUN_0047cee0): obtain a clipper, serialize a
 * client rectangle into region data, install it, attach it to the surface,
 * and release the temporary references. Callbacks use HRESULT convention. */

int directdraw_clipper_set_region(int width, int height)
{
    void* clipper = NULL;
    void* region_data = NULL;
    uint32_t region_size = 0u;
    int success = 0;
    if (s_directdraw_clipper_region_ops.clipper_get == NULL ||
        s_directdraw_clipper_region_ops.region_data_create == NULL ||
        s_directdraw_clipper_region_ops.clipper_region_set == NULL ||
        s_directdraw_clipper_region_ops.surface_clipper_set == NULL) {
        return 0;
    }
    if (s_directdraw_clipper_region_ops.clipper_get(
            s_directdraw_clipper_region_ops.context, &clipper) == 0 &&
        clipper != NULL &&
        s_directdraw_clipper_region_ops.region_data_create(
            s_directdraw_clipper_region_ops.context, width, height,
            &region_data, &region_size) == 0 && region_data != NULL &&
        s_directdraw_clipper_region_ops.clipper_region_set(
            s_directdraw_clipper_region_ops.context, clipper, region_data,
            region_size) == 0 &&
        s_directdraw_clipper_region_ops.surface_clipper_set(
            s_directdraw_clipper_region_ops.context, clipper) == 0) {
        success = 1;
    }
    if (region_data != NULL &&
        s_directdraw_clipper_region_ops.region_data_release != NULL) {
        s_directdraw_clipper_region_ops.region_data_release(
            s_directdraw_clipper_region_ops.context, region_data);
    }
    if (clipper != NULL &&
        s_directdraw_clipper_region_ops.clipper_release != NULL) {
        s_directdraw_clipper_region_ops.clipper_release(
            s_directdraw_clipper_region_ops.context, clipper);
    }
    return success;
}
/* SurfacePixelValueEncode (FUN_0047d2d0): resolve an RGB color through an
 * indexed palette at 8 bpp; otherwise scale each byte into its channel mask. */

uint32_t surface_pixel_value_encode(const SurfacePixelFormat* format,
                                    uint32_t color_rgb)
{
    uint32_t red = (color_rgb >> 16) & 0xffu;
    uint32_t green = (color_rgb >> 8) & 0xffu;
    uint32_t blue = color_rgb & 0xffu;
    if (format == NULL) return 0u;
    if (format->bits_per_pixel == 8u) {
        unsigned int i;
        if (format->palette_rgb == NULL) return 0u;
        for (i = 0; i < 256u; ++i) {
            uint32_t entry = format->palette_rgb[i];
            if (((entry >> 16) & 0xffu) == red &&
                ((entry >> 8) & 0xffu) == green &&
                (entry & 0xffu) == blue) {
                return i;
            }
        }
        return 0u;
    }
    else {
        const uint32_t red_bits = (uint32_t)pop_count32(format->red_mask);
        const uint32_t green_bits = (uint32_t)pop_count32(format->green_mask);
        const uint32_t blue_bits = (uint32_t)pop_count32(format->blue_mask);
        /* The retail encoder packs channels by their mask bit counts (red
         * above green above blue); it does not shift according to each
         * mask's first set bit. Standard DirectDraw RGB layouts produce the
         * same result either way, while this preserves the reference result
         * for unusual/non-contiguous masks too. */
        return ((red >> ((8u - red_bits) & 31u))
                << ((green_bits + blue_bits) & 31u)) |
               ((green >> ((8u - green_bits) & 31u))
                << (blue_bits & 31u)) |
               (blue >> ((8u - blue_bits) & 31u));
    }
}

/* FUN_0047d3f0: popcount via the clear-lowest-set-bit loop. */

int bit_count32(uint32_t value)
{
    int n = 0;
    while (value != 0u) {
        value &= value - 1u;
        ++n;
    }
    return n;
}

#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static int winapi_renderer_active(void)
{
    return s_winapi_renderer_state.device != NULL;
}
#endif
static int directdraw_device_create_internal(
    const void* guid, const void* adapter, int windowed, void** result_out)
{
    void* device = NULL;
    void* interface_pointer = NULL;
    void* result = NULL;
    int success = 0;
    if (s_directdraw_device_create_ops.device_create == NULL ||
        s_directdraw_device_create_ops.device_interface_query == NULL ||
        s_directdraw_device_create_ops.device_enumerate == NULL) return 0;
    if (s_directdraw_device_create_ops.device_create(
            s_directdraw_device_create_ops.context, guid, &device) == 0 &&
        device != NULL &&
        s_directdraw_device_create_ops.device_interface_query(
            s_directdraw_device_create_ops.context, device,
            &interface_pointer) == 0 && interface_pointer != NULL &&
        s_directdraw_device_create_ops.device_enumerate(
            s_directdraw_device_create_ops.context, device, interface_pointer,
            adapter, windowed, &result) == 0) {
        success = 1;
    }
    if (result_out != NULL) *result_out = result;
    if (interface_pointer != NULL &&
        s_directdraw_device_create_ops.release != NULL) {
        s_directdraw_device_create_ops.release(
            s_directdraw_device_create_ops.context, interface_pointer);
    }
    if (device != NULL && s_directdraw_device_create_ops.release != NULL) {
        s_directdraw_device_create_ops.release(
            s_directdraw_device_create_ops.context, device);
    }
    return success;
}

#if defined(LEGORRR_BACKEND_MINIWIN)
/* Native bodies of the DirectDraw.c entry points that lego_unit.c calls
 * (the unit is empty natively). Each keeps the behaviour of the port
 * adapter it replaces. */
// PORT-ONLY: native body of directdraw_clear (0x0047ce20); the port's colour fill
extern "C" void directdraw_clear(struct Area2F* window, unsigned int colour)
{
    (void)projection_overlay_draw_submit(window != NULL, NULL, NULL,
                                         (uint32_t)colour);
}

// PORT-ONLY: native body of directdraw_return_front_buffer (0x0047cc20); the native renderer keeps no front buffer
extern "C" void directdraw_return_front_buffer(void)
{
}

// PORT-ONLY: native body of directdraw_adjust_texture_usage (0x0047cd50)
extern "C" void directdraw_adjust_texture_usage(unsigned int* textureUsage)
{
    (void)textureUsage;
    query_texture_memory();
}

// PORT-ONLY: native body of directdraw_get_avail_texture_mem (0x0047cdd0)
extern "C" int directdraw_get_avail_texture_mem(unsigned int* total,
                                                unsigned int* avail)
{
    return surface_description_get((int*)total, (int*)avail);
}
#endif
