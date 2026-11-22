// SPDX-License-Identifier: LGPL-3.0-or-later
// Textures on miniwin's software renderer (ADR 0008, step 3), made the way
// Container_LoadTextureSurface makes them: the D3DRM device's preferred
// 8-bit palettized format, an 8-bit DDSCAPS_TEXTURE surface with a palette
// and a DDCKEY_SRCBLT colour key, then QueryInterface to IDirect3DTexture2.
// Mesh.c draws with it through SetTexture (or a TEXTUREHANDLE) and
// COLORKEYENABLE. Each case renders a textured quad over a blue one into a
// 64x64 target and reads the pixels back: the key colour leaves the blue
// visible, texture stage 0 modulates or selects, linear filtering does not
// bleed the key colour, and D3DRM's decal transparency does the same for
// retained-mode meshes.
// Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include "test_miniwin_device.h"

#include <functional>
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
static const int kTexSize = 8;
// Palette indices: the left half of the texture is red, the right half is
// the key colour (green).
static const BYTE kRed = 1;
static const BYTE kKey = 2;

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
    IDirect3DRMViewport2* viewport = nullptr;
    IDirect3DRMUserVisual* visual = nullptr;
};

static Scene g_scene;
static std::function<void(IDirect3DDevice3*)> g_draw;

static int Callback(
    IDirect3DRMUserVisual*,
    void*,
    D3DRMUSERVISUALREASON reason,
    IDirect3DRMDevice*,
    IDirect3DRMViewport*
)
{
    if (reason == D3DRMUSERVISUAL_RENDER && g_draw) {
        g_draw(g_scene.im);
    }
    return 1;
}

// ---- textures -------------------------------------------------------------

// Container_LoadTextureSurface's surface: 8-bit, palettized, keyed when
// key is set.
static IDirectDrawSurface4* MakeTextureSurface(bool key)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = kTexSize;
    desc.dwHeight = kTexSize;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    if (g_scene.device->FindPreferredTextureFormat(DDBD_8, D3DRMFPTF_PALETTIZED, &desc.ddpfPixelFormat) != DD_OK) {
        return nullptr;
    }
    IDirectDrawSurface4* surface = nullptr;
    if (g_scene.dd->CreateSurface(&desc, &surface, nullptr) != DD_OK || !surface) {
        return nullptr;
    }

    DDSURFACEDESC2 lock = {};
    lock.dwSize = sizeof(lock);
    CHECK(surface->Lock(nullptr, &lock, DDLOCK_WAIT, nullptr) == DD_OK);
    for (int y = 0; y < kTexSize; ++y) {
        BYTE* row = static_cast<BYTE*>(lock.lpSurface) + y * lock.lPitch;
        for (int x = 0; x < kTexSize; ++x) {
            row[x] = x < kTexSize / 2 ? kRed : kKey;
        }
    }
    surface->Unlock(nullptr);

    PALETTEENTRY entries[256] = {};
    entries[kRed].peRed = 255;
    entries[kKey].peGreen = 255;
    IDirectDrawPalette* palette = nullptr;
    CHECK(
        g_scene.dd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256, entries, &palette, nullptr) ==
            DD_OK &&
        palette
    );
    if (palette) {
        CHECK(surface->SetPalette(palette) == DD_OK);
        palette->Release();
    }
    if (key) {
        DDCOLORKEY colourKey = {kKey, kKey};
        CHECK(surface->SetColorKey(DDCKEY_SRCBLT, &colourKey) == DD_OK);
    }
    return surface;
}

static IDirect3DMaterial3* MakeMaterial(float r, float g, float b, D3DMATERIALHANDLE* handle)
{
    IDirect3DMaterial3* material = nullptr;
    if (g_scene.d3d->CreateMaterial(&material, nullptr) != DD_OK || !material) {
        return nullptr;
    }
    D3DMATERIAL data = {};
    data.dwSize = sizeof(data);
    data.diffuse = {r, g, b, 1.0f};
    data.ambient = {r, g, b, 1.0f};
    material->SetMaterial(&data);
    material->GetHandle(g_scene.im, handle);
    return material;
}

// A square of half-size s at depth z with texture coordinates 0..1, left to
// right and top to bottom, wound clockwise as seen from the camera.
static void DrawQuad(IDirect3DDevice3* im, float s, float z)
{
    D3DVERTEX v[4] = {
        {-s, s, z, 0, 0, -1, 0, 0},
        {s, s, z, 0, 0, -1, 1, 0},
        {-s, -s, z, 0, 0, -1, 0, 1},
        {s, -s, z, 0, 0, -1, 1, 1},
    };
    WORD indices[6] = {0, 1, 2, 2, 1, 3};
    CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, indices, 6, 0) == DD_OK);
}

// The blue backdrop, untextured, then (when there is a texture) the
// textured quad over the middle half of the target: x 16..47, its left half
// red and its right half key.
static void DrawScene(
    IDirect3DDevice3* im,
    D3DMATERIALHANDLE blue,
    D3DMATERIALHANDLE textured,
    IDirect3DTexture2* texture
)
{
    im->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(D3DCULL_CCW));
    im->SetTexture(0, nullptr);
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, blue);
    DrawQuad(im, 4.0f, 8.0f);
    if (texture) {
        im->SetTexture(0, texture);
        im->SetLightState(D3DLIGHTSTATE_MATERIAL, textured);
        DrawQuad(im, 1.0f, 4.0f);
        im->SetTexture(0, nullptr);
    }
}

// ---- reading back -------------------------------------------------------

struct RGB {
    int r, g, b;
};

// The back buffer is RGB565; channels come back scaled to 0..255.
static RGB Pixel(int x, int y)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    RGB out = {-1, -1, -1};
    if (g_scene.back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return out;
    }
    const Uint16* row = reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
    Uint16 p = row[x];
    g_scene.back->Unlock(nullptr);
    out.r = ((p >> 11) & 0x1f) * 255 / 31;
    out.g = ((p >> 5) & 0x3f) * 255 / 63;
    out.b = (p & 0x1f) * 255 / 31;
    return out;
}

static bool Near(const RGB& c, int r, int g, int b, int tolerance = 10)
{
    return abs(c.r - r) <= tolerance && abs(c.g - g) <= tolerance && abs(c.b - b) <= tolerance;
}

// Pixels on row y with any green in them.
static int GreenPixels(int y)
{
    int count = 0;
    for (int x = 0; x < kSize; ++x) {
        count += Pixel(x, y).g > 16 ? 1 : 0;
    }
    return count;
}

static void RenderFrame()
{
    static_cast<IDirect3DRMViewport*>(g_scene.viewport)->Clear();
    CHECK(g_scene.viewport->Render(g_scene.root) == DD_OK);
}

// ---- the cases ----------------------------------------------------------

static void CheckFormats()
{
    DDPIXELFORMAT format = {};
    format.dwSize = sizeof(format);
    CHECK(g_scene.device->FindPreferredTextureFormat(DDBD_8, D3DRMFPTF_PALETTIZED, &format) == DD_OK);
    CHECK(format.dwRGBBitCount == 8);
    CHECK((format.dwFlags & DDPF_PALETTEINDEXED8) == DDPF_PALETTEINDEXED8);

    format = {};
    format.dwSize = sizeof(format);
    CHECK(g_scene.device->FindPreferredTextureFormat(DDBD_16, 0, &format) == DD_OK);
    CHECK(format.dwRGBBitCount == 16);
    CHECK(format.dwRBitMask == 0xf800 && format.dwGBitMask == 0x07e0 && format.dwBBitMask == 0x001f);
    CHECK((format.dwFlags & DDPF_PALETTEINDEXED8) != DDPF_PALETTEINDEXED8);
}

static void CheckHandles(IDirect3DTexture2* texture, IDirect3DTexture2* other)
{
    IDirect3DDevice2* dev2 = nullptr;
    g_scene.im->QueryInterface(IID_IDirect3DDevice2, (void**) &dev2);
    D3DTEXTUREHANDLE a = 0, again = 0, b = 0;
    CHECK(texture->GetHandle(dev2, &a) == DD_OK && a != 0);
    CHECK(texture->GetHandle(dev2, &again) == DD_OK && again == a);
    CHECK(other->GetHandle(dev2, &b) == DD_OK && b != 0 && b != a);
    if (dev2) {
        dev2->Release();
    }
}

// The key colour leaves the blue backdrop visible while COLORKEYENABLE is
// on, and draws as itself while it is off.
static void CheckColourKey(D3DMATERIALHANDLE blue, D3DMATERIALHANDLE white, IDirect3DTexture2* texture)
{
    g_draw = [&](IDirect3DDevice3* im) {
        im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
        DrawScene(im, blue, white, texture);
    };
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 0, 255));
    CHECK(Near(Pixel(4, 32), 0, 0, 255));
    CHECK(GreenPixels(32) == 0);

    g_draw = [&](IDirect3DDevice3* im) {
        im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
        DrawScene(im, blue, white, texture);
    };
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 255, 0));
}

// Stage 0 modulates the texture with the lit diffuse colour by default;
// SELECTARG1 takes the texture alone and SELECTARG2 the diffuse colour.
static void CheckStageOps(D3DMATERIALHANDLE blue, D3DMATERIALHANDLE grey, IDirect3DTexture2* texture)
{
    auto draw = [&](DWORD colorOp) {
        g_draw = [&, colorOp](IDirect3DDevice3* im) {
            im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
            im->SetTextureStageState(0, D3DTSS_COLOROP, colorOp);
            DrawScene(im, blue, grey, texture);
            im->SetTextureStageState(0, D3DTSS_COLOROP, static_cast<DWORD>(D3DTOP_MODULATE));
        };
        RenderFrame();
    };
    draw(static_cast<DWORD>(D3DTOP_MODULATE));
    CHECK(Near(Pixel(24, 32), 128, 0, 0));
    // The untextured backdrop is not modulated by anything.
    CHECK(Near(Pixel(4, 32), 0, 0, 255));
    draw(static_cast<DWORD>(D3DTOP_SELECTARG1));
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    draw(static_cast<DWORD>(D3DTOP_SELECTARG2));
    CHECK(Near(Pixel(24, 32), 128, 128, 128));
    // A keyed texel is still not drawn when stage 0 takes only the diffuse
    // colour.
    CHECK(Near(Pixel(40, 32), 0, 0, 255));
}

// A TEXTUREHANDLE render state stands for the stage 0 texture when none is
// set.
static void CheckTextureHandle(D3DMATERIALHANDLE blue, D3DMATERIALHANDLE white, IDirect3DTexture2* texture)
{
    D3DTEXTUREHANDLE handle = 0;
    texture->GetHandle(nullptr, &handle);
    g_draw = [&](IDirect3DDevice3* im) {
        im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
        DrawScene(im, blue, white, nullptr);
        im->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, handle);
        im->SetLightState(D3DLIGHTSTATE_MATERIAL, white);
        DrawQuad(im, 1.0f, 4.0f);
        im->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
    };
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 0, 255));
}

// With linear filtering (Mesh.c's TEXTUREMAG LINEAR), the key colour does
// not bleed into the texels beside it.
static void CheckLinearFilter(D3DMATERIALHANDLE blue, D3DMATERIALHANDLE white, IDirect3DTexture2* texture)
{
    g_draw = [&](IDirect3DDevice3* im) {
        im->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
        im->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, static_cast<DWORD>(D3DFILTER_LINEAR));
        im->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, static_cast<DWORD>(D3DFILTER_MIPLINEAR));
        DrawScene(im, blue, white, texture);
        im->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, static_cast<DWORD>(D3DFILTER_NEAREST));
        im->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, static_cast<DWORD>(D3DFILTER_NEAREST));
    };
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 0, 255));
    for (int y = 18; y < 46; y += 3) {
        CHECK(GreenPixels(y) == 0);
    }
}

// Retained mode: Container_LoadTexture2 gives a D3DRM texture decal
// transparency in the key's colour instead of a surface colour key.
static void CheckDecalTransparency()
{
    IDirectDrawSurface4* surface = MakeTextureSurface(false);
    IDirectDrawSurface* surface1 = nullptr;
    IDirect3DRMTexture3* texture = nullptr;
    IDirect3DRMTexture* texture1 = nullptr;
    IDirect3DRMMesh* mesh = nullptr;
    IDirect3DRMFrame3* frame = nullptr;
    CHECK(surface != nullptr);
    if (surface) {
        surface->QueryInterface(IID_IDirectDrawSurface, (void**) &surface1);
    }
    CHECK(surface1 && g_scene.d3drm->CreateTextureFromSurface(surface1, &texture) == DD_OK && texture);
    CHECK(g_scene.d3drm->CreateMesh(&mesh) == DD_OK && mesh);
    CHECK(g_scene.d3drm->CreateFrame(g_scene.root, &frame) == DD_OK && frame);
    if (!texture || !mesh || !frame) {
        return;
    }
    texture->QueryInterface(IID_IDirect3DRMTexture, (void**) &texture1);

    unsigned int faces[6] = {0, 1, 2, 2, 1, 3};
    D3DRMGROUPINDEX group = 0;
    CHECK(mesh->AddGroup(4, 2, 3, faces, &group) == DD_OK);
    D3DRMVERTEX v[4] = {};
    const float s = 1.0f, z = 4.0f;
    const float pos[4][2] = {{-s, s}, {s, s}, {-s, -s}, {s, -s}};
    for (int i = 0; i < 4; ++i) {
        v[i].position = {pos[i][0], pos[i][1], z};
        v[i].normal = {0, 0, -1};
        v[i].tu = static_cast<float>(i & 1);
        v[i].tv = static_cast<float>(i >> 1);
    }
    CHECK(mesh->SetVertices(group, 0, 4, v) == DD_OK);
    mesh->SetGroupColorRGB(group, 1.0f, 1.0f, 1.0f);
    CHECK(mesh->SetGroupTexture(group, texture1) == DD_OK);
    frame->AddVisual(mesh);

    g_draw = nullptr;
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 255, 0));

    CHECK(texture->SetDecalTransparency(TRUE) == DD_OK);
    CHECK(texture->SetDecalTransparentColor(0x00ff00) == DD_OK);
    RenderFrame();
    CHECK(Near(Pixel(24, 32), 255, 0, 0));
    CHECK(Near(Pixel(40, 32), 0, 0, 0));

    CHECK(texture->SetDecalTransparency(FALSE) == DD_OK);
    RenderFrame();
    CHECK(Near(Pixel(40, 32), 0, 255, 0));

    frame->DeleteVisual(mesh);
    g_scene.root->DeleteChild(frame);
    IUnknown* objects[] = {frame, mesh, texture1, texture, surface1, surface};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
}

// ---- set-up ---------------------------------------------------------------

static HRESULT FindSoftwareDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, MiniwinTestDevice()) == 0) {
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

static bool SetUp(SDL_Window* window)
{
    Scene& s = g_scene;
    IDirectDraw* dd1 = nullptr;
    if (DirectDrawCreate(nullptr, &dd1, nullptr) != DD_OK || !dd1) {
        return false;
    }
    dd1->QueryInterface(IID_IDirectDraw4, (void**) &s.dd);
    dd1->QueryInterface(IID_IDirect3D3, (void**) &s.d3d);
    if (!s.dd || !s.d3d) {
        return false;
    }
    s.dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL);

    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    if (s.dd->CreateSurface(&desc, &s.front, nullptr) != DD_OK || !s.front) {
        return false;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = kSize;
    desc.dwHeight = kSize;
    if (s.dd->CreateSurface(&desc, &s.back, nullptr) != DD_OK || !s.back) {
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
    if (MiniwinTestDeviceMissing(software)) {
        return false;
    }

    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1) {
        return false;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &s.d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    s.back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    if (!s.d3drm || !back1 || s.d3drm->CreateDeviceFromSurface(&software, dd1, back1, 0, &s.device) != DD_OK) {
        return false;
    }
    back1->Release();
    dd1->Release();

    IDirect3DDevice2* im2 = nullptr;
    s.device->GetDirect3DDevice2(&im2);
    if (!im2) {
        return false;
    }
    im2->QueryInterface(IID_IDirect3DDevice3, (void**) &s.im);
    im2->Release();
    if (!s.im) {
        return false;
    }

    s.d3drm->CreateFrame(nullptr, &s.root);
    s.d3drm->CreateFrame(s.root, &s.camera);
    s.d3drm->CreateFrame(s.root, &s.object);
    if (!s.root || !s.camera || !s.object) {
        return false;
    }

    // A white ambient light: lit colours are the material's diffuse colour.
    IDirect3DRMLight* light = nullptr;
    s.d3drm->CreateLightRGB(D3DRMLIGHT_AMBIENT, 1.0f, 1.0f, 1.0f, &light);
    if (!light) {
        return false;
    }
    s.root->AddLight(light);
    light->Release();

    if (s.d3drm->CreateViewport(s.device, s.camera, 0, 0, kSize, kSize, &s.viewport) != DD_OK || !s.viewport) {
        return false;
    }
    s.viewport->SetFront(1.0f);
    s.viewport->SetBack(100.0f);
    s.viewport->SetField(0.5f);

    if (s.d3drm->CreateUserVisual(Callback, nullptr, &s.visual) != DD_OK || !s.visual) {
        return false;
    }
    s.object->AddVisual(s.visual);
    return true;
}

static void TearDown()
{
    Scene& s = g_scene;
    IUnknown* objects[] = {s.visual, s.viewport, s.object, s.camera, s.root, s.im, s.device, s.d3drm, s.back, s.front, s.d3d, s.dd};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
    s = Scene();
}

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, MiniwinTestVideoDriver());
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_textures", kSize, kSize, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    if (!SetUp(window)) {
        if (g_miniwinTestDeviceMissing) {
            return kMiniwinTestSkip;
        }
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    CheckFormats();

    IDirectDrawSurface4* keyed = MakeTextureSurface(true);
    IDirectDrawSurface4* plain = MakeTextureSurface(false);
    IDirect3DTexture2* texture = nullptr;
    IDirect3DTexture2* other = nullptr;
    CHECK(keyed && keyed->QueryInterface(IID_IDirect3DTexture2, (void**) &texture) == DD_OK && texture);
    CHECK(plain && plain->QueryInterface(IID_IDirect3DTexture2, (void**) &other) == DD_OK && other);

    D3DMATERIALHANDLE blue = 0, white = 0, grey = 0;
    IDirect3DMaterial3* materials[] = {
        MakeMaterial(0, 0, 1, &blue),
        MakeMaterial(1, 1, 1, &white),
        MakeMaterial(0.5f, 0.5f, 0.5f, &grey),
    };
    CHECK(blue && white && grey);

    if (texture && other) {
        CheckHandles(texture, other);
        CheckColourKey(blue, white, texture);
        CheckStageOps(blue, grey, texture);
        CheckTextureHandle(blue, white, texture);
        CheckLinearFilter(blue, white, texture);
    }
    if (MiniwinTestRetainedAdditions("CheckDecalTransparency")) {
        CheckDecalTransparency();
    }

    g_draw = nullptr;
    for (IDirect3DMaterial3* material : materials) {
        if (material) {
            material->Release();
        }
    }
    IUnknown* objects[] = {texture, other, keyed, plain};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
    TearDown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin textures: all checks passed\n");
    return 0;
}
