// SPDX-License-Identifier: LGPL-3.0-or-later
// Viewports and clears on miniwin's software renderer (ADR 0008, step 2),
// as the level and its radar use them: two Direct3DRM viewports on one
// 64x64 target, the radar's a sub-rectangle. Checks GetX/GetY, Clear2 and
// Viewport::Clear(flags) rectangles and colours, that nothing a viewport
// draws leaves its rectangle, that its projection is centred on it, and
// that once the game clears the z-buffer itself, Render no longer does.
// Transform and InverseTransform use D3DRM's homogeneous screen vectors in
// device coordinates, as the game's terrain picking reads them.
// Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <functional>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static const int kSize = 64;

static IDirectDraw4* g_dd;
static IDirect3D3* g_d3d;
static IDirectDrawSurface4* g_front;
static IDirectDrawSurface4* g_back;
static IDirect3DRM3* g_d3drm;
static IDirect3DRMDevice3* g_device;
static IDirect3DDevice3* g_im;
static IDirect3DRMFrame3* g_root;
static IDirect3DRMFrame3* g_camera;
static std::function<void(IDirect3DDevice3*)> g_draw;

static int Callback(IDirect3DRMUserVisual*, void*, D3DRMUSERVISUALREASON reason, IDirect3DRMDevice*, IDirect3DRMViewport*)
{
    if (reason == D3DRMUSERVISUAL_RENDER && g_draw) {
        g_draw(g_im);
    }
    return 1;
}

struct RGB {
    int r, g, b;
};

static RGB Pixel(int x, int y)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    RGB out = {-1, -1, -1};
    if (g_back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return out;
    }
    const Uint16* row = reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
    Uint16 p = row[x];
    g_back->Unlock(nullptr);
    out = {((p >> 11) & 0x1f) * 255 / 31, ((p >> 5) & 0x3f) * 255 / 63, (p & 0x1f) * 255 / 31};
    return out;
}

static bool Is(const RGB& c, int r, int g, int b)
{
    return abs(c.r - r) <= 10 && abs(c.g - g) <= 10 && abs(c.b - b) <= 10;
}

// True when every pixel of the rectangle [x1, x2) x [y1, y2) has the colour.
static bool AllAre(int x1, int y1, int x2, int y2, int r, int g, int b)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (g_back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return false;
    }
    bool all = true;
    for (int y = y1; y < y2 && all; ++y) {
        const Uint16* row =
            reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
        for (int x = x1; x < x2 && all; ++x) {
            Uint16 p = row[x];
            all = Is({((p >> 11) & 0x1f) * 255 / 31, ((p >> 5) & 0x3f) * 255 / 63, (p & 0x1f) * 255 / 31}, r, g, b);
        }
    }
    g_back->Unlock(nullptr);
    return all;
}

static IDirect3DViewport3* ImViewport(IDirect3DRMViewport2* viewport)
{
    IDirect3DViewport* im1 = nullptr;
    IDirect3DViewport3* im = nullptr;
    if (viewport->GetDirect3DViewport(&im1) == DD_OK && im1) {
        im1->QueryInterface(IID_IDirect3DViewport3, (void**) &im);
        im1->Release();
    }
    return im;
}

static void Clear2(IDirect3DRMViewport2* viewport, D3DRECT rect, DWORD flags, D3DCOLOR color)
{
    IDirect3DViewport3* im = ImViewport(viewport);
    CHECK(im && im->Clear2(1, &rect, flags, color, 1.0f, 0) == DD_OK);
    if (im) {
        im->Release();
    }
}

static D3DMATERIALHANDLE Material(float r, float g, float b, IDirect3DMaterial3** keep)
{
    D3DMATERIALHANDLE handle = 0;
    if (g_d3d->CreateMaterial(keep, nullptr) == DD_OK && *keep) {
        D3DMATERIAL data = {};
        data.dwSize = sizeof(data);
        data.diffuse = {r, g, b, 1};
        data.ambient = {r, g, b, 1};
        (*keep)->SetMaterial(&data);
        (*keep)->GetHandle(g_im, &handle);
    }
    return handle;
}

// A square of half-size s at depth z, clockwise as the camera sees it.
static void Quad(IDirect3DDevice3* im, D3DMATERIALHANDLE material, float s, float z)
{
    D3DVERTEX v[4] = {
        {-s, s, z, 0, 0, -1, 0, 0},
        {s, s, z, 0, 0, -1, 1, 0},
        {-s, -s, z, 0, 0, -1, 0, 1},
        {s, -s, z, 0, 0, -1, 1, 1},
    };
    WORD indices[6] = {0, 1, 2, 2, 1, 3};
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, material);
    CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, indices, 6, 0) == DD_OK);
}

static HRESULT FindSoftwareDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, "Miniwin Emulation") == 0) {
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

static bool SetUp(SDL_Window* window)
{
    IDirectDraw* dd1 = nullptr;
    if (DirectDrawCreate(nullptr, &dd1, nullptr) != DD_OK || !dd1) {
        return false;
    }
    dd1->QueryInterface(IID_IDirectDraw4, (void**) &g_dd);
    dd1->QueryInterface(IID_IDirect3D3, (void**) &g_d3d);
    if (!g_dd || !g_d3d) {
        return false;
    }
    g_dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL);
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    if (g_dd->CreateSurface(&desc, &g_front, nullptr) != DD_OK) {
        return false;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = kSize;
    desc.dwHeight = kSize;
    if (g_dd->CreateSurface(&desc, &g_back, nullptr) != DD_OK) {
        return false;
    }
    GUID software = {};
    IDirect3D2* d3d2 = nullptr;
    dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2);
    if (!d3d2) {
        return false;
    }
    d3d2->EnumDevices(FindSoftwareDevice, &software);
    d3d2->Release();
    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK) {
        return false;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &g_d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    g_back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    if (!g_d3drm || !back1 || g_d3drm->CreateDeviceFromSurface(&software, dd1, back1, 0, &g_device) != DD_OK) {
        return false;
    }
    back1->Release();
    dd1->Release();
    IDirect3DDevice2* im2 = nullptr;
    g_device->GetDirect3DDevice2(&im2);
    if (!im2) {
        return false;
    }
    im2->QueryInterface(IID_IDirect3DDevice3, (void**) &g_im);
    im2->Release();

    g_d3drm->CreateFrame(nullptr, &g_root);
    g_d3drm->CreateFrame(g_root, &g_camera);
    IDirect3DRMLight* light = nullptr;
    g_d3drm->CreateLightRGB(D3DRMLIGHT_AMBIENT, 1.0f, 1.0f, 1.0f, &light);
    if (!g_im || !g_root || !g_camera || !light) {
        return false;
    }
    g_root->AddLight(light);
    light->Release();
    IDirect3DRMUserVisual* visual = nullptr;
    g_d3drm->CreateUserVisual(Callback, nullptr, &visual);
    if (!visual) {
        return false;
    }
    g_root->AddVisual(visual);
    visual->Release();
    return true;
}

static IDirect3DRMViewport2* MakeViewport(DWORD x, DWORD y, DWORD w, DWORD h)
{
    IDirect3DRMViewport2* viewport = nullptr;
    CHECK(g_d3drm->CreateViewport(g_device, g_camera, x, y, w, h, &viewport) == DD_OK && viewport);
    if (viewport) {
        viewport->SetFront(1.0f);
        viewport->SetBack(100.0f);
        viewport->SetField(0.5f);
    }
    return viewport;
}

static bool Close(float a, float b, float tolerance = 1e-3f)
{
    return fabsf(a - b) <= tolerance * (1.0f + fabsf(b));
}

// D3DRM screen vectors: x / w and y / w are device pixels, z / w the depth
// from 0 at the front plane to 1 at the back one. Both viewports see from a
// camera at the origin looking down +z with front 1, back 100 and field 0.5,
// so a point at depth 10 is a fifth of the half-view off centre per unit.
static void CheckTransforms(IDirect3DRMViewport2* level, IDirect3DRMViewport2* radar)
{
    // The view matrix is the one the last Render used.
    CHECK(level->Render(g_root) == DD_OK);
    CHECK(radar->Render(g_root) == DD_OK);

    D3DVECTOR centre = {0, 0, 10};
    D3DRMVECTOR4D s = {};
    CHECK(level->Transform(&s, &centre) == DD_OK);
    CHECK(Close(s.w, 10) && Close(s.x / s.w, 32) && Close(s.y / s.w, 32));
    CHECK(Close(s.z / s.w, 100.0f / 99.0f * (1.0f - 1.0f / 10.0f)));

    // In the radar the same offsets land around its own centre, (48, 16).
    D3DVECTOR corner = {1.25f, 1.25f, 10};
    CHECK(radar->Transform(&s, &corner) == DD_OK);
    CHECK(Close(s.x / s.w, 52) && Close(s.y / s.w, 12));

    D3DVECTOR front = {0, 0, 1}, back = {0, 0, 100};
    CHECK(level->Transform(&s, &front) == DD_OK && Close(s.z / s.w, 0));
    CHECK(level->Transform(&s, &back) == DD_OK && Close(s.z / s.w, 1));

    // InverseTransform undoes Transform, whatever the w.
    for (IDirect3DRMViewport2* viewport : {level, radar}) {
        D3DVECTOR p = {-3, 2, 25}, q = {};
        CHECK(viewport->Transform(&s, &p) == DD_OK);
        CHECK(viewport->InverseTransform(&q, &s) == DD_OK);
        CHECK(Close(q.x, p.x) && Close(q.y, p.y) && Close(q.z, p.z));
        D3DRMVECTOR4D unit = {s.x / s.w, s.y / s.w, s.z / s.w, 1};
        CHECK(viewport->InverseTransform(&q, &unit) == DD_OK);
        CHECK(Close(q.x, p.x) && Close(q.y, p.y) && Close(q.z, p.z));
    }

    // terrain_map_screen_pick's ray: the mouse at depth 0 and at depth 1.
    D3DRMVECTOR4D nearScreen = {52, 12, 0, 1}, farScreen = {52, 12, 1, 1};
    D3DVECTOR nearPoint = {}, farPoint = {};
    CHECK(radar->InverseTransform(&nearPoint, &nearScreen) == DD_OK);
    CHECK(radar->InverseTransform(&farPoint, &farScreen) == DD_OK);
    CHECK(Close(nearPoint.x, 0.125f) && Close(nearPoint.y, 0.125f) && Close(nearPoint.z, 1));
    CHECK(Close(farPoint.x, 12.5f) && Close(farPoint.y, 12.5f) && Close(farPoint.z, 100));
}

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_viewports", kSize, kSize, 0);
    if (!window || !SetUp(window)) {
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    // The level fills the device; the radar is the top-right quarter.
    IDirect3DRMViewport2* level = MakeViewport(0, 0, kSize, kSize);
    IDirect3DRMViewport2* radar = MakeViewport(32, 0, 32, 32);
    if (!level || !radar) {
        return 1;
    }
    CHECK(level->GetX() == 0 && level->GetY() == 0);
    CHECK(radar->GetX() == 32 && radar->GetY() == 0);
    CHECK(radar->GetWidth() == 32 && radar->GetHeight() == 32);

    IDirect3DMaterial3* keep[3] = {};
    D3DMATERIALHANDLE red = Material(1, 0, 0, &keep[0]);
    D3DMATERIALHANDLE green = Material(0, 1, 0, &keep[1]);
    D3DMATERIALHANDLE blue = Material(0, 0, 1, &keep[2]);

    // Clear2 colours exactly its rectangle, clipped to the viewport.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff);
    Clear2(level, {8, 8, 24, 24}, D3DCLEAR_TARGET, 0xffff0000);
    CHECK(AllAre(8, 8, 24, 24, 255, 0, 0));
    CHECK(Is(Pixel(7, 8), 0, 0, 255) && Is(Pixel(24, 24), 0, 0, 255));
    Clear2(radar, {0, 0, kSize, kSize}, D3DCLEAR_TARGET, 0xff00ff00); // clipped to the radar
    CHECK(AllAre(32, 0, 64, 32, 0, 255, 0));
    CHECK(Is(Pixel(31, 0), 0, 0, 255) && Is(Pixel(32, 32), 0, 0, 255));

    // Viewport::Clear(flags) fills its own rectangle with the scene's
    // background colour.
    g_root->SetSceneBackground(0xff00ffff);
    CHECK(radar->Clear(D3DRMCLEAR_TARGET) == DD_OK);
    CHECK(AllAre(32, 0, 64, 32, 0, 255, 255));
    CHECK(Is(Pixel(16, 48), 0, 0, 255));
    g_root->SetSceneBackground(0xff000000);

    // A quad bigger than the view stays inside the radar's rectangle.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000);
    g_draw = [&](IDirect3DDevice3* im) { Quad(im, red, 50, 10); };
    CHECK(radar->Render(g_root) == DD_OK);
    CHECK(AllAre(32, 0, 64, 32, 255, 0, 0));
    CHECK(AllAre(0, 0, 32, 64, 0, 0, 0) && AllAre(32, 32, 64, 64, 0, 0, 0));

    // Its projection is centred on the rectangle: a quad a quarter of the
    // view wide covers the middle of the radar, not of the device.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000);
    g_draw = [&](IDirect3DDevice3* im) { Quad(im, green, 1.25f, 10); };
    CHECK(radar->Render(g_root) == DD_OK);
    CHECK(AllAre(45, 13, 51, 19, 0, 255, 0));
    CHECK(Is(Pixel(36, 4), 0, 0, 0) && Is(Pixel(32, 32), 0, 0, 0));

    // The level viewport still draws over the whole device afterwards.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000);
    g_draw = [&](IDirect3DDevice3* im) { Quad(im, blue, 50, 10); };
    CHECK(level->Render(g_root) == DD_OK);
    CHECK(AllAre(0, 0, kSize, kSize, 0, 0, 255));

    // The game clears z itself, so Render keeps the z-buffer: a far quad
    // drawn in the next Render stays behind the near one drawn before.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000);
    g_draw = [&](IDirect3DDevice3* im) { Quad(im, green, 1, 5); };
    CHECK(level->Render(g_root) == DD_OK);
    g_draw = [&](IDirect3DDevice3* im) { Quad(im, red, 50, 10); };
    CHECK(level->Render(g_root) == DD_OK);
    CHECK(Is(Pixel(32, 32), 0, 255, 0) && Is(Pixel(2, 2), 255, 0, 0));
    // After a z clear the far quad covers it.
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_ZBUFFER, 0);
    CHECK(Is(Pixel(32, 32), 0, 255, 0)); // a z-only clear leaves the colour
    CHECK(level->Render(g_root) == DD_OK);
    CHECK(Is(Pixel(32, 32), 255, 0, 0));

    // The view cone matches the projection for any front plane. With front
    // 0.1 and field 0.5 the view shows x / z up to 5, so a quad at x / z 2
    // to 3 is on screen, at pixels 45 to 51.
    level->SetFront(0.1f);
    Clear2(level, {0, 0, kSize, kSize}, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000);
    g_draw = [&](IDirect3DDevice3* im) {
        D3DVERTEX v[4] = {
            {20, 10, 10, 0, 0, -1, 0, 0},
            {30, 10, 10, 0, 0, -1, 1, 0},
            {20, -10, 10, 0, 0, -1, 0, 1},
            {30, -10, 10, 0, 0, -1, 1, 1},
        };
        WORD indices[6] = {0, 1, 2, 2, 1, 3};
        im->SetLightState(D3DLIGHTSTATE_MATERIAL, green);
        CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, indices, 6, 0) == DD_OK);
    };
    CHECK(level->Render(g_root) == DD_OK);
    CHECK(AllAre(46, 28, 50, 36, 0, 255, 0));
    CHECK(Is(Pixel(32, 32), 0, 0, 0));
    level->SetFront(1.0f);

    g_draw = nullptr;
    CheckTransforms(level, radar);
    for (IDirect3DMaterial3* material : keep) {
        if (material) {
            material->Release();
        }
    }
    radar->Release();
    level->Release();
    IUnknown* objects[] = {g_camera, g_root, g_im, g_device, g_d3drm, g_back, g_front, g_d3d, g_dd};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin viewports and clears: all checks passed\n");
    return 0;
}
