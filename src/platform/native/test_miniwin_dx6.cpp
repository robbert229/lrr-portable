// SPDX-License-Identifier: LGPL-3.0-or-later
// The DirectX 6 interface versions the units call (ADR 0007, decision 3)
// are declared in miniwin. Each is reached by QueryInterface or a Create
// method from an object miniwin creates. The methods miniwin does not
// implement yet must return a failure code rather than crash or succeed;
// the ones it implements return success here and are exercised in
// test_miniwin_ddraw4.cpp.
// Synthetic only: an offscreen SDL window, no game data.
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>

#include <SDL3/SDL.h>

#include <stdio.h>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

// The object answers riid; the returned pointer is released again.
static bool Answers(IUnknown* object, const GUID& riid)
{
    void* out = nullptr;
    if (object->QueryInterface(riid, &out) != DD_OK || !out) {
        return false;
    }
    // Every miniwin interface derives IUnknown virtually, so the
    // reference is released through the object, not through out.
    object->Release();
    return true;
}

static int UserVisualCallback(IDirect3DRMUserVisual*, void*,
                              D3DRMUSERVISUALREASON, IDirect3DRMDevice*,
                              IDirect3DRMViewport*)
{
    return 0;
}

static HRESULT FindSoftwareDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC,
                                  LPD3DDEVICEDESC, LPVOID ctx)
{
    GUID* found = static_cast<GUID*>(ctx);
    if (SDL_strcmp(name, "Miniwin Emulation") == 0) {
        *found = *guid;
    }
    return DDENUMRET_OK;
}

static void CheckDirectDraw(IDirectDraw* dd, IDirectDrawSurface** outSurface)
{
    CHECK(Answers(dd, IID_IDirectDraw));
    CHECK(Answers(dd, IID_IDirectDraw2));
    IDirectDraw4* dd4 = nullptr;
    CHECK(dd->QueryInterface(IID_IDirectDraw4, (void**) &dd4) == DD_OK && dd4);
    if (dd4) {
        DDSURFACEDESC2 desc2 = {};
        desc2.dwSize = sizeof(desc2);
        IDirectDrawSurface4* surface4 = nullptr;
        DDSCAPS2 caps2 = {};
        DWORD total = 0, free = 0;
        // Behaviour is covered by test_miniwin_ddraw4.cpp.
        CHECK(dd4->CreateSurface(&desc2, &surface4, nullptr) == DDERR_INVALIDPARAMS);
        CHECK(dd4->EnumDisplayModes(0, nullptr, nullptr, nullptr) == DDERR_INVALIDPARAMS);
        CHECK(dd4->SetDisplayMode(64, 64, 32, 0, 0) == DD_OK);
        CHECK(dd4->GetAvailableVidMem(&caps2, &total, &free) == DD_OK && total && free);
        dd4->Release();
    }

    IDirectDrawClipper* clipper = nullptr;
    CHECK(dd->CreateClipper(0, &clipper, nullptr) == DD_OK && clipper);
    if (clipper) {
        CHECK(clipper->SetClipList(nullptr, 0) == DD_OK);
        clipper->Release();
    }

    DDSURFACEDESC desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.dwWidth = 64;
    desc.dwHeight = 64;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    IDirectDrawSurface* surface = nullptr;
    CHECK(dd->CreateSurface(&desc, &surface, nullptr) == DD_OK && surface);
    *outSurface = surface;
    if (!surface) {
        return;
    }
    CHECK(Answers(surface, IID_IDirectDrawSurface));
    CHECK(Answers(surface, IID_IDirectDrawSurface3));
    IDirectDrawSurface4* surface4 = nullptr;
    CHECK(surface->QueryInterface(IID_IDirectDrawSurface4, (void**) &surface4) == DD_OK && surface4);
    if (surface4) {
        DDSURFACEDESC2 desc2 = {};
        desc2.dwSize = sizeof(desc2);
        DDSCAPS2 caps2 = {};
        IDirectDrawSurface4* attached = nullptr;
        CHECK(surface4->Blt(nullptr, surface4, nullptr, DDBLT_WAIT, nullptr) == DD_OK);
        CHECK(surface4->Flip(nullptr, DDFLIP_WAIT) == DDERR_NOTFLIPPABLE);
        CHECK(surface4->GetAttachedSurface(&caps2, &attached) == DDERR_NOTFOUND);
        CHECK(surface4->GetSurfaceDesc(&desc2) == DD_OK && desc2.dwWidth == 64);
        CHECK(surface4->Lock(nullptr, &desc2, DDLOCK_WAIT, nullptr) == DD_OK && desc2.lpSurface);
        CHECK(surface4->Unlock(nullptr) == DD_OK);
        surface4->Release();
    }
    IDirect3DTexture2* texture2 = nullptr;
    CHECK(surface->QueryInterface(IID_IDirect3DTexture2, (void**) &texture2) == DD_OK && texture2);
    if (texture2) {
        D3DTEXTUREHANDLE handle = 0;
        CHECK(texture2->GetHandle(nullptr, &handle) == DD_OK && handle != 0);
        texture2->Release();
    }
}

static void CheckDirect3D(IDirectDraw* dd)
{
    IDirect3D3* d3d3 = nullptr;
    CHECK(dd->QueryInterface(IID_IDirect3D3, (void**) &d3d3) == DD_OK && d3d3);
    if (!d3d3) {
        return;
    }
    IDirect3DMaterial3* material = nullptr;
    CHECK(d3d3->CreateMaterial(&material, nullptr) == DD_OK && material);
    if (material) {
        CHECK(Answers(material, IID_IDirect3DMaterial3));
        D3DMATERIAL data = {};
        data.dwSize = sizeof(data);
        D3DMATERIALHANDLE handle = 0;
        CHECK(material->SetMaterial(&data) == DD_OK);
        CHECK(material->GetHandle(nullptr, &handle) == DD_OK && handle != 0);
        material->Release();
    }
    d3d3->Release();
}

static void CheckDevice(IDirect3DRMDevice3* device)
{
    CHECK(Answers(device, IID_IDirect3DRMDevice));
    CHECK(Answers(device, IID_IDirect3DRMDevice2));
    CHECK(Answers(device, IID_IDirect3DRMDevice3));
    DDPIXELFORMAT format = {};
    CHECK(device->FindPreferredTextureFormat(DDBD_16, 0, &format) == DD_OK && format.dwRGBBitCount == 16);

    IDirect3DDevice2* imDevice2 = nullptr;
    CHECK(device->GetDirect3DDevice2(&imDevice2) == DD_OK && imDevice2);
    if (!imDevice2) {
        return;
    }
    IDirect3DDevice3* imDevice = nullptr;
    CHECK(imDevice2->QueryInterface(IID_IDirect3DDevice3, (void**) &imDevice) == DD_OK && imDevice);
    imDevice2->Release();
    if (!imDevice) {
        return;
    }
    DWORD value = 0;
    D3DMATRIX matrix = {};
    WORD indices[3] = {0, 1, 2};
    float vertices[3 * 8] = {};
    IDirect3DTexture2* texture = nullptr;
    IDirect3D3* d3d = nullptr;
    CHECK(imDevice->GetDirect3D(&d3d) == DD_OK && d3d);
    if (d3d) {
        d3d->Release();
    }
    // The device keeps its immediate-mode state (test_miniwin_immediate
    // draws with it).
    CHECK(imDevice->SetCurrentViewport(nullptr) == DD_OK);
    CHECK(imDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, &value) == DD_OK && value == TRUE);
    CHECK(imDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, 0) == DD_OK);
    CHECK(imDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, &value) == DD_OK && value == 0);
    CHECK(imDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, 1) == DD_OK);
    CHECK(imDevice->SetLightState(D3DLIGHTSTATE_AMBIENT, 0x00404040) == DD_OK);
    CHECK(imDevice->GetLightState(D3DLIGHTSTATE_AMBIENT, &value) == DD_OK && value == 0x00404040);
    matrix._11 = matrix._22 = matrix._33 = matrix._44 = 2.0f;
    CHECK(imDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &matrix) == DD_OK);
    D3DMATRIX got = {};
    CHECK(imDevice->GetTransform(D3DTRANSFORMSTATE_WORLD, &got) == DD_OK && got._11 == 2.0f && got._12 == 0.0f);
    // Only D3DFVF_VERTEX triangle lists are drawn.
    CHECK(imDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vertices, 3, indices, 3, 0) ==
          DDERR_UNSUPPORTED);
    CHECK(imDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX, vertices, 3, indices, 3, 0) ==
          DDERR_UNSUPPORTED);
    WORD outOfRange[3] = {0, 1, 3};
    CHECK(imDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, vertices, 3, outOfRange, 3, 0) ==
          DDERR_INVALIDPARAMS);
    CHECK(imDevice->GetTexture(0, &texture) == DD_OK && texture == nullptr);
    CHECK(imDevice->SetTexture(0, nullptr) == DD_OK);
    CHECK(imDevice->GetTextureStageState(0, D3DTSS_COLOROP, &value) == DD_OK && value == 4);
    CHECK(imDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2) == DD_OK);
    CHECK(imDevice->GetTextureStageState(0, D3DTSS_COLOROP, &value) == DD_OK && value == 2);
    CHECK(imDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4) == DD_OK);
    matrix._11 = matrix._22 = matrix._33 = matrix._44 = 1.0f;
    CHECK(imDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &matrix) == DD_OK);
    imDevice->Release();
}

static void CheckViewport(IDirect3DRM3* d3drm, IDirect3DRMDevice3* device, IDirect3DRMFrame3* camera)
{
    IDirect3DRMViewport2* viewport = nullptr;
    CHECK(d3drm->CreateViewport(device, camera, 0, 0, 64, 64, &viewport) == DD_OK && viewport);
    if (!viewport) {
        return;
    }
    CHECK(Answers(viewport, IID_IDirect3DRMViewport));
    CHECK(Answers(viewport, IID_IDirect3DRMViewport2));
    IDirect3DRMFrame3* got = nullptr;
    CHECK(viewport->GetCamera(&got) == DD_OK && got == camera);
    if (got) {
        got->Release();
    }
    CHECK(viewport->Clear(D3DRMCLEAR_ALL) == DD_OK);
    CHECK(viewport->GetX() == 0);
    CHECK(viewport->GetY() == 0);

    IDirect3DViewport* imViewport1 = nullptr;
    CHECK(viewport->GetDirect3DViewport(&imViewport1) == DD_OK && imViewport1);
    if (imViewport1) {
        IDirect3DViewport3* imViewport = nullptr;
        CHECK(imViewport1->QueryInterface(IID_IDirect3DViewport3, (void**) &imViewport) == DD_OK && imViewport);
        if (imViewport) {
            D3DRECT rect = {0, 0, 64, 64};
            CHECK(imViewport->Clear2(1, &rect, 0, 0, 1.0f, 0) == DD_OK);
            imViewport->Release();
        }
        imViewport1->Release();
    }
    viewport->Release();
}

static void CheckFrame(IDirect3DRM3* d3drm, IDirect3DRMFrame3** outCamera)
{
    IDirect3DRMFrame3* frame = nullptr;
    CHECK(d3drm->CreateFrame(nullptr, &frame) == DD_OK && frame);
    *outCamera = frame;
    if (!frame) {
        return;
    }
    CHECK(Answers(frame, IID_IDirect3DRMFrame));
    CHECK(Answers(frame, IID_IDirect3DRMFrame2));
    CHECK(Answers(frame, IID_IDirect3DRMFrame3));

    // The version-3 overloads that only widen pointer types forward to
    // the existing implementation.
    IDirect3DRMFrame3* child = nullptr;
    CHECK(d3drm->CreateFrame(nullptr, &child) == DD_OK && child);
    D3DVECTOR position = {};
    CHECK(frame->AddChild(child) == DD_OK);
    CHECK(child->GetPosition(nullptr, &position) == DD_OK);
    CHECK(frame->DeleteChild(child) == DD_OK);

    D3DRMMATRIX4D matrix = {};
    D3DVECTOR dir = {}, up = {};
    DWORD count = 0;
    IDirect3DRMFrame3* other = nullptr;
    // The transform methods are covered by test_miniwin_ddraw4.cpp.
    CHECK(frame->AddTranslation(D3DRMCOMBINE_BEFORE, 1, 2, 3) == DD_OK);
    CHECK(frame->AddScale(D3DRMCOMBINE_BEFORE, 1, 1, 1) == DD_OK);
    CHECK(frame->AddRotation(D3DRMCOMBINE_BEFORE, 0, 1, 0, 1) == DD_OK);
    CHECK(frame->AddVisual(static_cast<IUnknown*>(child)) == DD_OK);
    CHECK(frame->GetParent(&other) == DD_OK && other == nullptr);
    CHECK(frame->GetScene(&other) == DD_OK && other == frame);
    if (other) {
        other->Release();
    }
    CHECK(frame->GetTransform(nullptr, matrix) == DD_OK);
    CHECK(frame->GetOrientation(nullptr, &dir, &up) == DD_OK);
    CHECK(frame->GetVisuals(&count, nullptr) == DD_OK && count == 1);
    CHECK(frame->InverseTransform(&dir, &up) == DD_OK);
    CHECK(frame->Load(nullptr, nullptr, D3DRMLOAD_FROMFILE, nullptr, nullptr) == E_NOTIMPL);
    CHECK(frame->LookAt(child, nullptr, D3DRMCONSTRAIN_Z) == E_NOTIMPL);
    CHECK(frame->DeleteVisual(static_cast<IUnknown*>(child)) == DD_OK);
    CHECK(frame->GetSceneBackground() == 0xFF000000);
    CHECK(frame->SetSceneBackground(0xFF102030) == DD_OK);
    CHECK(frame->GetSceneBackground() == 0xFF102030);
    CHECK(frame->SetSceneFogEnable(TRUE) == DD_OK);
    CHECK(frame->SetSceneFogColor(0) == DD_OK);
    CHECK(frame->SetSceneFogMode(D3DRMFOG_LINEAR) == DD_OK);
    CHECK(frame->SetSceneFogParams(1, 10, 1) == DD_OK);
    CHECK(frame->SetSceneFogMethod(D3DRMFOGMETHOD_VERTEX) == DD_OK);
    CHECK(frame->SetOrientation(nullptr, 0, 0, 1, 0, 1, 0) == DD_OK);
    CHECK(frame->SetPosition(nullptr, 1, 2, 3) == DD_OK);
    CHECK(frame->SetSortMode(D3DRMSORT_NONE) == DD_OK);
    CHECK(frame->Transform(&dir, &up) == DD_OK);
    if (child) {
        child->Release();
    }
}

static void CheckRetainedMode(IDirectDraw* dd, IDirectDrawSurface* surface)
{
    IDirect3DRM* d3drm1 = nullptr;
    CHECK(Direct3DRMCreate(&d3drm1) == DD_OK && d3drm1);
    if (!d3drm1) {
        return;
    }
    CHECK(Answers(d3drm1, IID_IDirect3DRM));
    CHECK(Answers(d3drm1, IID_IDirect3DRM2));
    IDirect3DRM3* d3drm = nullptr;
    CHECK(d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &d3drm) == DD_OK && d3drm);
    d3drm1->Release();
    if (!d3drm) {
        return;
    }

    IDirect3DRMFrame3* camera = nullptr;
    CheckFrame(d3drm, &camera);

    IDirect3DRMTexture3* loaded = nullptr;
    CHECK(d3drm->LoadTexture("synthetic.bmp", &loaded) == E_NOTIMPL);

    IDirect3DRMMaterial2* material = nullptr;
    CHECK(d3drm->CreateMaterial(10.0f, &material) == DD_OK && material);
    if (material) {
        CHECK(Answers(material, IID_IDirect3DRMMaterial));
        CHECK(Answers(material, IID_IDirect3DRMMaterial2));
        CHECK(material->GetPower() == 10.0f);
        D3DVALUE r = 0, g = 0, b = 0;
        CHECK(material->GetSpecular(&r, &g, &b) == DD_OK && r == 1 && g == 1 && b == 1);
        CHECK(material->SetSpecular(0.25f, 0.5f, 0.75f) == DD_OK);
        CHECK(material->GetSpecular(&r, &g, &b) == DD_OK && r == 0.25f && g == 0.5f && b == 0.75f);
        CHECK(material->SetEmissive(0.5f, 0, 1) == DD_OK);
        CHECK(material->GetEmissive(&r, &g, &b) == DD_OK && r == 0.5f && g == 0 && b == 1);
        material->Release();
    }

    IDirect3DRMMeshBuilder3* builder = nullptr;
    CHECK(d3drm->CreateMeshBuilder(&builder) == DD_OK && builder);
    if (builder) {
        CHECK(Answers(builder, IID_IDirect3DRMMeshBuilder));
        CHECK(Answers(builder, IID_IDirect3DRMMeshBuilder2));
        CHECK(Answers(builder, IID_IDirect3DRMMeshBuilder3));
        IDirect3DRMMesh* mesh = nullptr;
        CHECK(builder->Load(nullptr, nullptr, D3DRMLOAD_FROMMEMORY, nullptr, nullptr) == E_NOTIMPL);
        CHECK(builder->SetPerspective(TRUE) == E_NOTIMPL);
        CHECK(builder->CreateMesh(&mesh) == E_NOTIMPL);
        builder->Release();
    }

    IDirect3DRMAnimationSet2* animationSet = nullptr;
    CHECK(d3drm->CreateAnimationSet(&animationSet) == DD_OK && animationSet);
    if (animationSet) {
        CHECK(Answers(animationSet, IID_IDirect3DRMAnimationSet2));
        CHECK(animationSet->Load(nullptr, nullptr, D3DRMLOAD_FROMFILE, nullptr, nullptr, camera) == E_NOTIMPL);
        CHECK(animationSet->SetTime(0.5f) == E_NOTIMPL);
        animationSet->Release();
    }

    IDirect3DRMUserVisual* userVisual = nullptr;
    CHECK(d3drm->CreateUserVisual(UserVisualCallback, nullptr, &userVisual) == DD_OK && userVisual);
    if (userVisual) {
        int tag = 0;
        CHECK(Answers(userVisual, IID_IDirect3DRMUserVisual));
        CHECK(userVisual->SetAppData(&tag) == DD_OK);
        CHECK(userVisual->GetAppData() == &tag);
        userVisual->Release();
    }

    IDirect3DRMMesh* mesh = nullptr;
    CHECK(d3drm->CreateMesh(&mesh) == DD_OK && mesh);
    if (mesh) {
        CHECK(mesh->Scale(2, 2, 2) == E_NOTIMPL);
        mesh->Release();
    }

    IDirect3DRMLight* light = nullptr;
    CHECK(d3drm->CreateLightRGB(D3DRMLIGHT_SPOT, 1, 1, 1, &light) == DD_OK && light);
    if (light) {
        // D3DRM's defaults, then the set values.
        CHECK(light->GetRange() == 256.0f && light->GetUmbra() == 0.4f && light->GetPenumbra() == 0.5f);
        CHECK(light->GetConstantAttenuation() == 1 && light->GetLinearAttenuation() == 0);
        CHECK(light->GetQuadraticAttenuation() == 0);
        CHECK(light->SetColor(0x80ff4000) == DD_OK && light->GetColor() == 0xffff4000);
        CHECK(light->SetRange(10) == DD_OK && light->GetRange() == 10);
        CHECK(light->SetUmbra(0.5f) == DD_OK && light->GetUmbra() == 0.5f);
        CHECK(light->SetPenumbra(0.6f) == DD_OK && light->GetPenumbra() == 0.6f);
        IDirect3DRMFrame* enable = nullptr;
        CHECK(light->GetEnableFrame(&enable) == DD_OK && enable == nullptr);
        CHECK(light->SetEnableFrame(camera) == DD_OK);
        CHECK(light->GetEnableFrame(&enable) == DD_OK && enable == camera);
        if (enable) {
            enable->Release();
        }
        CHECK(light->SetEnableFrame(nullptr) == DD_OK);
        light->Release();
    }

    IDirect3DRMTexture3* texture = nullptr;
    CHECK(surface && d3drm->CreateTextureFromSurface(surface, &texture) == DD_OK && texture);
    if (texture) {
        CHECK(Answers(texture, IID_IDirect3DRMTexture));
        CHECK(Answers(texture, IID_IDirect3DRMTexture2));
        CHECK(Answers(texture, IID_IDirect3DRMTexture3));
        CHECK(texture->GetImage() == nullptr);
        CHECK(texture->SetDecalTransparency(TRUE) == DD_OK);
        CHECK(texture->SetDecalTransparentColor(0) == DD_OK);
        CHECK(texture->GenerateMIPMap(0) == E_NOTIMPL);
        texture->Release();
    }

    // A device needs a renderer: the software one, on the offscreen window.
    IDirect3D2* d3d2 = nullptr;
    GUID software = {};
    CHECK(dd->QueryInterface(IID_IDirect3D2, (void**) &d3d2) == DD_OK && d3d2);
    if (d3d2) {
        d3d2->EnumDevices(FindSoftwareDevice, &software);
        d3d2->Release();
    }
    IDirect3DRMDevice3* device = nullptr;
    // Device flags (D3DRMDEVICE_NOZBUFFER) are not supported yet.
    CHECK(surface && d3drm->CreateDeviceFromSurface(&software, dd, surface, 1, &device) == E_NOTIMPL);
    CHECK(surface && d3drm->CreateDeviceFromSurface(&software, dd, surface, 0, &device) == DD_OK && device);
    if (device) {
        CheckDevice(device);
        if (camera) {
            CheckViewport(d3drm, device, camera);
        }
    }
    if (camera) {
        camera->Release();
    }
    d3drm->Release();
}

int main()
{
    // The stubs log once each; keep the test output to its own checks.
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_dx6", 64, 64, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    IDirectDraw* dd = nullptr;
    CHECK(DirectDrawCreate(nullptr, &dd, nullptr) == DD_OK && dd);
    if (dd) {
        CHECK(dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL) == DD_OK);
        IDirectDrawSurface* surface = nullptr;
        CheckDirectDraw(dd, &surface);
        CheckDirect3D(dd);
        CheckRetainedMode(dd, surface);
        if (surface) {
            surface->Release();
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin DX6 interfaces: all checks passed\n");
    return 0;
}
