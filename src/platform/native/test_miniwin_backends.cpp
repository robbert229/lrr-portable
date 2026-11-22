// SPDX-License-Identifier: LGPL-3.0-or-later
// The SDL GPU backend against the software backend (ADR 0008, step 6): one
// scene drawn the way gods98's Mesh.c draws, on each device in turn, compared
// pixel by pixel. The scene has lit, specular and table-fogged Gouraud
// geometry, a colour-keyed and bilinear-filtered texture, additive and
// SRCALPHA blending, and a second viewport in a sub-rectangle with its own
// Clear2, as the level and its radar have.
//
// The software backend is the reference. Lighting, culling and vertex fog
// run on the CPU for both, so colours agree to rounding; the rasterizers'
// edge rules do not agree exactly. The tolerance:
//  - a pixel matches when every channel is within kChannelTolerance of the
//    software pixel (RGB565 steps of 8, plus rounding in blends and fog);
//  - a pixel that does not match must lie on an edge: within one pixel of a
//    place where the software image itself changes by more than
//    kChannelTolerance between neighbours;
//  - at most kMaxEdgeMismatch of the target's pixels may mismatch.
//
// Synthetic only. Skipped (77) where miniwin offers no "SDL3 GPU HAL" device
// that can draw on SDL's offscreen video driver, as on CI; Mesa's lavapipe
// (VK_DRIVER_FILES=.../lvp_icd.x86_64.json) provides one without a GPU.
#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include "test_miniwin_device.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

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
static const int kTexSize = 8;
static const int kChannelTolerance = 12;
static const double kMaxEdgeMismatch = 0.04;

struct RGB {
    int r, g, b;
};

struct Scene {
    IDirectDraw4* dd = nullptr;
    IDirect3D3* d3d = nullptr;
    IDirectDrawSurface4* front = nullptr;
    IDirectDrawSurface4* back = nullptr;
    IDirect3DRM3* d3drm = nullptr;
    IDirect3DRMDevice3* device = nullptr;
    IDirect3DDevice3* im = nullptr;
    IDirect3DRMFrame3* root = nullptr;
    IDirect3DRMFrame3* camera = nullptr;
    IDirect3DRMFrame3* object = nullptr;
    IDirect3DRMViewport2* level = nullptr;
    IDirect3DRMViewport2* radar = nullptr;
    IDirect3DRMUserVisual* visual = nullptr;
    IDirectDrawSurface4* textureSurface = nullptr;
    IDirect3DTexture2* texture = nullptr;
    IDirect3DMaterial3* materials[4] = {};
    D3DMATERIALHANDLE shiny = 0, white = 0, red = 0, halfGreen = 0;
};

static Scene g_scene;

// ---- the scene ------------------------------------------------------------

// A 6x6 grid bulging toward the camera at depth z, with normals that turn
// across it, so that lighting varies from vertex to vertex.
static void DrawBulge(IDirect3DDevice3* im, float s, float z)
{
    const int n = 6;
    D3DVERTEX v[(n + 1) * (n + 1)];
    WORD indices[n * n * 6];
    int k = 0;
    for (int j = 0; j <= n; ++j) {
        for (int i = 0; i <= n; ++i) {
            float x = -s + 2 * s * i / n, y = s - 2 * s * j / n;
            float nx = x / (2 * s), ny = y / (2 * s);
            float nz = -sqrtf(fmaxf(0.0f, 1 - nx * nx - ny * ny));
            float bulge = 1.5f * nz;
            v[k++] = {x, y, z + bulge, nx, ny, nz, 0, 0};
        }
    }
    k = 0;
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            WORD a = j * (n + 1) + i, b = a + 1, c = a + n + 1, d = c + 1;
            WORD quad[6] = {a, b, c, c, b, d};
            for (WORD index : quad) {
                indices[k++] = index;
            }
        }
    }
    CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, (n + 1) * (n + 1), indices, k, 0) == DD_OK);
}

// A square of half-size s centred on (cx, cy) at depth z, texture
// coordinates 0..2 so the texture repeats, wound clockwise.
static void DrawQuad(IDirect3DDevice3* im, float cx, float cy, float s, float z)
{
    D3DVERTEX v[4] = {
        {cx - s, cy + s, z, 0, 0, -1, 0, 0},
        {cx + s, cy + s, z, 0, 0, -1, 2, 0},
        {cx - s, cy - s, z, 0, 0, -1, 0, 2},
        {cx + s, cy - s, z, 0, 0, -1, 2, 2},
    };
    WORD indices[6] = {0, 1, 2, 2, 1, 3};
    CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, indices, 6, 0) == DD_OK);
}

static void SetFloatState(IDirect3DDevice3* im, D3DRENDERSTATETYPE state, float value)
{
    DWORD bits;
    SDL_memcpy(&bits, &value, sizeof(bits));
    im->SetRenderState(state, bits);
}

static void DrawScene(IDirect3DDevice3* im)
{
    Scene& s = g_scene;
    im->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(D3DCULL_CCW));
    im->SetRenderState(D3DRENDERSTATE_SHADEMODE, static_cast<DWORD>(D3DSHADE_GOURAUD));
    im->SetTexture(0, nullptr);

    // Lit, specular and table-fogged.
    im->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_FOGCOLOR, 0x00406080);
    im->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, static_cast<DWORD>(D3DFOG_LINEAR));
    SetFloatState(im, D3DRENDERSTATE_FOGTABLESTART, 6.0f);
    SetFloatState(im, D3DRENDERSTATE_FOGTABLEEND, 14.0f);
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, s.shiny);
    DrawBulge(im, 4.0f, 10.0f);
    im->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
    im->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);

    // Keyed and filtered: the key texels show what is behind.
    im->SetTexture(0, s.texture);
    im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, static_cast<DWORD>(D3DFILTER_LINEAR));
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, s.white);
    DrawQuad(im, -1.2f, 1.0f, 1.2f, 6.0f);
    im->SetTexture(0, nullptr);
    im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);

    // Additive, as Mesh.c's ALPHATRANS, and SRCALPHA over INVSRCALPHA.
    im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_ONE));
    im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_ONE));
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, s.red);
    DrawQuad(im, 1.0f, -0.5f, 1.0f, 5.0f);
    im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_SRCALPHA));
    im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_INVSRCALPHA));
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, s.halfGreen);
    DrawQuad(im, -0.5f, -1.0f, 1.0f, 4.5f);
    im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
}

static int Callback(
    IDirect3DRMUserVisual*,
    void*,
    D3DRMUSERVISUALREASON reason,
    IDirect3DRMDevice*,
    IDirect3DRMViewport*
)
{
    if (reason == D3DRMUSERVISUAL_RENDER) {
        DrawScene(g_scene.im);
    }
    return 1;
}

static void Clear2(IDirect3DRMViewport2* viewport, D3DRECT rect, D3DCOLOR color)
{
    IDirect3DViewport* d3dViewport1 = nullptr;
    viewport->GetDirect3DViewport(&d3dViewport1);
    IDirect3DViewport3* d3dViewport = nullptr;
    if (d3dViewport1) {
        d3dViewport1->QueryInterface(IID_IDirect3DViewport3, (void**) &d3dViewport);
        d3dViewport1->Release();
    }
    CHECK(d3dViewport != nullptr);
    if (d3dViewport) {
        CHECK(d3dViewport->Clear2(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0) == DD_OK);
        d3dViewport->Release();
    }
}

// Level.c's frame: clear and render the level, then the radar in its
// rectangle.
static void RenderFrame()
{
    Scene& s = g_scene;
    Clear2(s.level, {0, 0, kSize, kSize}, 0xff102030);
    CHECK(s.level->Render(s.root) == DD_OK);
    Clear2(s.radar, {40, 4, 60, 24}, 0xff300000);
    CHECK(s.radar->Render(s.root) == DD_OK);
}

// ---- set-up ---------------------------------------------------------------

// An 8-bit keyed checkerboard: white, grey and key texels.
static IDirectDrawSurface4* MakeTextureSurface()
{
    Scene& s = g_scene;
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = kTexSize;
    desc.dwHeight = kTexSize;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    if (s.device->FindPreferredTextureFormat(DDBD_8, D3DRMFPTF_PALETTIZED, &desc.ddpfPixelFormat) != DD_OK) {
        return nullptr;
    }
    IDirectDrawSurface4* surface = nullptr;
    if (s.dd->CreateSurface(&desc, &surface, nullptr) != DD_OK || !surface) {
        return nullptr;
    }
    DDSURFACEDESC2 lock = {};
    lock.dwSize = sizeof(lock);
    if (surface->Lock(nullptr, &lock, DDLOCK_WAIT, nullptr) == DD_OK) {
        for (int y = 0; y < kTexSize; ++y) {
            BYTE* row = static_cast<BYTE*>(lock.lpSurface) + y * lock.lPitch;
            for (int x = 0; x < kTexSize; ++x) {
                row[x] = static_cast<BYTE>(((x / 2 + y / 2) % 3) + 1); // 1 white, 2 grey, 3 key
            }
        }
        surface->Unlock(nullptr);
    }
    PALETTEENTRY entries[256] = {};
    const BYTE colours[4][3] = {{0, 0, 0}, {255, 255, 255}, {128, 96, 64}, {0, 255, 0}};
    for (int i = 1; i < 4; ++i) {
        entries[i].peRed = colours[i][0];
        entries[i].peGreen = colours[i][1];
        entries[i].peBlue = colours[i][2];
    }
    IDirectDrawPalette* palette = nullptr;
    s.dd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256, entries, &palette, nullptr);
    if (palette) {
        surface->SetPalette(palette);
        palette->Release();
    }
    DDCOLORKEY key = {3, 3};
    surface->SetColorKey(DDCKEY_SRCBLT, &key);
    return surface;
}

static IDirect3DMaterial3* MakeMaterial(const D3DMATERIAL& values, D3DMATERIALHANDLE* handle)
{
    IDirect3DMaterial3* material = nullptr;
    if (g_scene.d3d->CreateMaterial(&material, nullptr) != DD_OK || !material) {
        return nullptr;
    }
    D3DMATERIAL data = values;
    data.dwSize = sizeof(data);
    material->SetMaterial(&data);
    material->GetHandle(g_scene.im, handle);
    return material;
}

struct Find {
    const char* name;
    GUID guid;
    bool found;
};

static HRESULT FindDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    Find* find = static_cast<Find*>(ctx);
    if (SDL_strcmp(name, find->name) == 0) {
        find->guid = *guid;
        find->found = true;
    }
    return DDENUMRET_OK;
}

enum class SetUpResult {
    Ok,
    Missing,
    Failed
};

static SetUpResult SetUp(SDL_Window* window, const char* deviceName)
{
    Scene& s = g_scene;
    IDirectDraw* dd1 = nullptr;
    if (DirectDrawCreate(nullptr, &dd1, nullptr) != DD_OK || !dd1) {
        return SetUpResult::Failed;
    }
    dd1->QueryInterface(IID_IDirectDraw4, (void**) &s.dd);
    dd1->QueryInterface(IID_IDirect3D3, (void**) &s.d3d);
    if (!s.dd || !s.d3d) {
        return SetUpResult::Failed;
    }
    s.dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL);

    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    if (s.dd->CreateSurface(&desc, &s.front, nullptr) != DD_OK || !s.front) {
        return SetUpResult::Failed;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = kSize;
    desc.dwHeight = kSize;
    if (s.dd->CreateSurface(&desc, &s.back, nullptr) != DD_OK || !s.back) {
        return SetUpResult::Failed;
    }

    Find find = {deviceName, {}, false};
    IDirect3D2* d3d2 = nullptr;
    dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2);
    if (!d3d2) {
        return SetUpResult::Failed;
    }
    d3d2->EnumDevices(FindDevice, &find);
    d3d2->Release();
    if (!find.found) {
        dd1->Release();
        return SetUpResult::Missing;
    }

    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1) {
        return SetUpResult::Failed;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &s.d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    s.back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    HRESULT created = s.d3drm && back1 ? s.d3drm->CreateDeviceFromSurface(&find.guid, dd1, back1, 0, &s.device)
                                       : DDERR_GENERIC;
    if (back1) {
        back1->Release();
    }
    dd1->Release();
    if (created != DD_OK) {
        // Offered, but it cannot draw on this video driver (a GPU that
        // cannot present to an offscreen surface).
        return SetUpResult::Missing;
    }

    IDirect3DDevice2* im2 = nullptr;
    s.device->GetDirect3DDevice2(&im2);
    if (!im2) {
        return SetUpResult::Failed;
    }
    im2->QueryInterface(IID_IDirect3DDevice3, (void**) &s.im);
    im2->Release();
    if (!s.im) {
        return SetUpResult::Failed;
    }

    s.d3drm->CreateFrame(nullptr, &s.root);
    s.d3drm->CreateFrame(s.root, &s.camera);
    s.d3drm->CreateFrame(s.root, &s.object);
    if (!s.root || !s.camera || !s.object) {
        return SetUpResult::Failed;
    }

    IDirect3DRMLight* ambient = nullptr;
    s.d3drm->CreateLightRGB(D3DRMLIGHT_AMBIENT, 0.25f, 0.25f, 0.3f, &ambient);
    IDirect3DRMLight* sun = nullptr;
    s.d3drm->CreateLightRGB(D3DRMLIGHT_DIRECTIONAL, 0.9f, 0.85f, 0.7f, &sun);
    IDirect3DRMFrame3* sunFrame = nullptr;
    s.d3drm->CreateFrame(s.root, &sunFrame);
    if (!ambient || !sun || !sunFrame) {
        return SetUpResult::Failed;
    }
    s.root->AddLight(ambient);
    sunFrame->SetOrientation(s.root, 0.5f, -0.6f, 1.0f, 0.0f, 1.0f, 0.0f);
    sunFrame->AddLight(sun);
    ambient->Release();
    sun->Release();
    sunFrame->Release();

    if (s.d3drm->CreateViewport(s.device, s.camera, 0, 0, kSize, kSize, &s.level) != DD_OK || !s.level) {
        return SetUpResult::Failed;
    }
    if (s.d3drm->CreateViewport(s.device, s.camera, 40, 4, 20, 20, &s.radar) != DD_OK || !s.radar) {
        return SetUpResult::Failed;
    }
    for (IDirect3DRMViewport2* viewport : {s.level, s.radar}) {
        viewport->SetFront(1.0f);
        viewport->SetBack(100.0f);
        viewport->SetField(0.5f);
    }
    if (s.d3drm->CreateUserVisual(Callback, nullptr, &s.visual) != DD_OK || !s.visual) {
        return SetUpResult::Failed;
    }
    s.object->AddVisual(s.visual);

    s.textureSurface = MakeTextureSurface();
    if (!s.textureSurface ||
        s.textureSurface->QueryInterface(IID_IDirect3DTexture2, (void**) &s.texture) != DD_OK) {
        return SetUpResult::Failed;
    }

    D3DMATERIAL shiny = {};
    shiny.diffuse = {0.8f, 0.7f, 0.6f, 1.0f};
    shiny.ambient = {0.8f, 0.7f, 0.6f, 1.0f};
    shiny.specular = {1.0f, 1.0f, 1.0f, 1.0f};
    shiny.power = 8.0f;
    D3DMATERIAL white = {};
    white.diffuse = white.ambient = {1, 1, 1, 1};
    D3DMATERIAL red = {};
    red.diffuse = red.ambient = {0.4f, 0.1f, 0.0f, 1};
    D3DMATERIAL halfGreen = {};
    halfGreen.diffuse = halfGreen.ambient = {0.1f, 0.9f, 0.2f, 0.5f};
    s.materials[0] = MakeMaterial(shiny, &s.shiny);
    s.materials[1] = MakeMaterial(white, &s.white);
    s.materials[2] = MakeMaterial(red, &s.red);
    s.materials[3] = MakeMaterial(halfGreen, &s.halfGreen);
    return s.shiny && s.white && s.red && s.halfGreen ? SetUpResult::Ok : SetUpResult::Failed;
}

static void TearDown()
{
    Scene& s = g_scene;
    for (IDirect3DMaterial3* material : s.materials) {
        if (material) {
            material->Release();
        }
    }
    IUnknown* objects[] = {s.texture, s.textureSurface, s.visual, s.radar, s.level, s.object, s.camera, s.root,
                           s.im,      s.device,         s.d3drm,  s.back,  s.front, s.d3d,    s.dd};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
    s = Scene();
}

// The back buffer, RGB565, with channels scaled to 0..255.
static std::vector<RGB> ReadBack()
{
    std::vector<RGB> pixels(kSize * kSize, RGB{-1, -1, -1});
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (g_scene.back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return pixels;
    }
    for (int y = 0; y < kSize; ++y) {
        const Uint16* row =
            reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
        for (int x = 0; x < kSize; ++x) {
            Uint16 p = row[x];
            pixels[y * kSize + x] = {((p >> 11) & 0x1f) * 255 / 31, ((p >> 5) & 0x3f) * 255 / 63, (p & 0x1f) * 255 / 31};
        }
    }
    g_scene.back->Unlock(nullptr);
    return pixels;
}

static SetUpResult Render(const char* deviceName, const char* title, std::vector<RGB>& pixels)
{
    SDL_Window* window = SDL_CreateWindow(title, kSize, kSize, 0);
    if (!window) {
        // The video driver cannot open a window here (macOS runners have no
        // offscreen GL for SDL's offscreen driver).
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return SetUpResult::Missing;
    }
    SetUpResult result = SetUp(window, deviceName);
    if (result == SetUpResult::Ok) {
        RenderFrame();
        pixels = ReadBack();
    }
    TearDown();
    SDL_DestroyWindow(window);
    return result;
}

static int Difference(const RGB& a, const RGB& b)
{
    return SDL_max(abs(a.r - b.r), SDL_max(abs(a.g - b.g), abs(a.b - b.b)));
}

// MINIWIN_BACKENDS_DUMP=<prefix> writes <prefix>-<name>.bmp, scaled up 4x,
// to look at the two images side by side.
static void Dump(const std::vector<RGB>& pixels, const char* name)
{
    const char* prefix = SDL_getenv("MINIWIN_BACKENDS_DUMP");
    if (!prefix || !*prefix) {
        return;
    }
    const int scale = 4;
    SDL_Surface* image = SDL_CreateSurface(kSize * scale, kSize * scale, SDL_PIXELFORMAT_RGB24);
    if (!image) {
        return;
    }
    for (int y = 0; y < kSize * scale; ++y) {
        Uint8* row = static_cast<Uint8*>(image->pixels) + y * image->pitch;
        for (int x = 0; x < kSize * scale; ++x) {
            const RGB& p = pixels[(y / scale) * kSize + x / scale];
            row[x * 3] = static_cast<Uint8>(p.r);
            row[x * 3 + 1] = static_cast<Uint8>(p.g);
            row[x * 3 + 2] = static_cast<Uint8>(p.b);
        }
    }
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s-%s.bmp", prefix, name);
    SDL_SaveBMP(image, path);
    SDL_DestroySurface(image);
}

// ---- the comparison -------------------------------------------------------

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    // MINIWIN_TEST_VIDEO_DRIVER picks another driver, for a GPU that cannot
    // present to an offscreen surface.
    const char* driver = SDL_getenv("MINIWIN_TEST_VIDEO_DRIVER");
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, driver && *driver ? driver : "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s (skipped)\n", SDL_GetError());
        return kMiniwinTestSkip;
    }

    std::vector<RGB> reference, gpu;
    SetUpResult software = Render("Miniwin Emulation", "miniwin_backends software", reference);
    if (software == SetUpResult::Missing) {
        fprintf(stderr, "no window on this video driver: skipped\n");
        SDL_Quit();
        return kMiniwinTestSkip;
    }
    if (software != SetUpResult::Ok) {
        fprintf(stderr, "software set-up failed\n");
        return 1;
    }
    SetUpResult result = Render("SDL3 GPU HAL", "miniwin_backends sdlgpu", gpu);
    if (result == SetUpResult::Missing) {
        fprintf(stderr, "no SDL3 GPU HAL device that draws offscreen: skipped\n");
        SDL_Quit();
        return kMiniwinTestSkip;
    }
    if (result != SetUpResult::Ok) {
        fprintf(stderr, "SDL GPU set-up failed\n");
        return 1;
    }

    Dump(reference, "software");
    Dump(gpu, "sdlgpu");

    // The scene draws something in each of its parts.
    const RGB background = {16, 32, 48};
    int drawn = 0;
    for (const RGB& p : reference) {
        drawn += Difference(p, background) > kChannelTolerance ? 1 : 0;
    }
    CHECK(drawn > kSize * kSize / 4);

    // Where the software image changes sharply: edges, within one pixel.
    std::vector<bool> edge(kSize * kSize, false);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const RGB& p = reference[y * kSize + x];
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < kSize && ny < kSize &&
                        Difference(p, reference[ny * kSize + nx]) > kChannelTolerance) {
                        edge[y * kSize + x] = true;
                    }
                }
            }
        }
    }

    int mismatched = 0, interior = 0, worst = 0;
    for (int i = 0; i < kSize * kSize; ++i) {
        int d = Difference(reference[i], gpu[i]);
        worst = SDL_max(worst, d);
        if (d > kChannelTolerance) {
            ++mismatched;
            if (!edge[i]) {
                ++interior;
                if (interior <= 8) {
                    const RGB& a = reference[i];
                    const RGB& b = gpu[i];
                    fprintf(stderr, "  interior (%d, %d): software %d,%d,%d gpu %d,%d,%d\n", i % kSize, i / kSize,
                            a.r, a.g, a.b, b.r, b.g, b.b);
                }
            }
        }
    }
    printf("miniwin backends: %d of %d pixels differ by more than %d (%d off an edge), largest difference %d\n",
           mismatched, kSize * kSize, kChannelTolerance, interior, worst);
    CHECK(interior == 0);
    CHECK(mismatched <= static_cast<int>(kMaxEdgeMismatch * kSize * kSize));

    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin backends: SDL GPU matches software within tolerance\n");
    return 0;
}
