// SPDX-License-Identifier: LGPL-3.0-or-later
/* Generic mesh groups and texture ownership, using only invented geometry. */
#include <cstdlib>
#include <fstream>
#include <vector>

#include "platform/native/test_fixture.h"
#include <miniwin/ddraw.h>
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/files.h"
#include "gods98/main_globs.h"
#include "gods98/mesh_link_names.h"
#include "gods98/mesh_api.h"
#include "test/lrr_test.h"

extern "C" int main_init_app(HINSTANCE);

namespace {
bool completed;

void exit_guard()
{
    if (!completed) std::_Exit(1);
}

bool write_texture()
{
    // 4x4 indexed BMP with an original grayscale palette and solid pixels.
    constexpr unsigned offset = 54 + 256 * 4;
    std::vector<unsigned char> bytes(offset + 16, 0);
    auto put = [&](unsigned at, uint32_t value, unsigned count) {
        for (unsigned i = 0; i < count; ++i)
            bytes[at + i] = static_cast<unsigned char>(value >> (i * 8));
    };
    bytes[0] = 'B';
    bytes[1] = 'M';
    put(2, static_cast<uint32_t>(bytes.size()), 4);
    put(10, offset, 4);
    put(14, 40, 4);
    put(18, 4, 4);
    put(22, 4, 4);
    put(26, 1, 2);
    put(28, 8, 2);
    put(34, 16, 4);
    put(46, 256, 4);
    for (unsigned i = 0; i < 256; ++i)
        for (unsigned c = 0; c < 3; ++c)
            bytes[54 + 4 * i + c] = static_cast<unsigned char>(i);
    for (unsigned i = offset; i < bytes.size(); ++i) bytes[i] = 128;
    std::ofstream out("Data/gray.bmp", std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();
    return out.good();
}

D3DRMVERTEX vertex(float x, float y, float z, float u, float v)
{
    D3DRMVERTEX result = {};
    result.position = {x, y, z};
    result.normal = {0, 0, 1};
    result.tu = u;
    result.tv = v;
    return result;
}

void check_vertex(const D3DRMVERTEX& actual, const D3DRMVERTEX& expected)
{
    CHECK_NEAR(actual.position.x, expected.position.x, 1e-6f);
    CHECK_NEAR(actual.position.y, expected.position.y, 1e-6f);
    CHECK_NEAR(actual.position.z, expected.position.z, 1e-6f);
    CHECK_NEAR(actual.normal.x, expected.normal.x, 1e-6f);
    CHECK_NEAR(actual.normal.y, expected.normal.y, 1e-6f);
    CHECK_NEAR(actual.normal.z, expected.normal.z, 1e-6f);
    CHECK_NEAR(actual.tu, expected.tu, 1e-6f);
    CHECK_NEAR(actual.tv, expected.tv, 1e-6f);
}

void destroyed(IDirect3DRMObject*, void* count)
{
    ++*static_cast<unsigned*>(count);
}

void check_retained(Container* root)
{
    Container* mesh = Container_MakeMesh2(root, 1); // separate retained groups
    if (!CHECK(mesh && mesh->typeData && mesh->typeData->mesh, "create retained mesh")) return;
    unsigned int triangle[] = {0, 1, 2};
    unsigned int quad[] = {0, 1, 2, 3};
    if (!CHECK_EQ(Container_Mesh_AddGroup(mesh, 3, 1, 3, triangle), 0ul) ||
        !CHECK_EQ(Container_Mesh_AddGroup(mesh, 4, 1, 4, quad), 1ul)) {
        Container_Remove(mesh);
        return;
    }
    CHECK_EQ(Container_Mesh_GetGroupCount(mesh), 2ul);
    D3DRMVERTEX tri[] = {
        vertex(-2, 0, 1, 0, 0), vertex(1, 0, 1, 1, 0), vertex(0, 3, 1, 0.5f, 1)
    };
    D3DRMVERTEX rectangle[] = {
        vertex(4, -1, -2, 0, 0), vertex(7, -1, -2, 1, 0),
        vertex(7, 2, -2, 1, 1), vertex(4, 2, -2, 0, 1)
    };
    CHECK_EQ(Container_Mesh_SetVertices(mesh, 0, 0, 3, tri), 0ul);
    CHECK_EQ(Container_Mesh_SetVertices(mesh, 1, 0, 4, rectangle), 0ul);
    auto updated = vertex(8, 2, -2, 0.75f, 0.5f);
    CHECK_EQ(Container_Mesh_SetVertices(mesh, 1, 2, 1, &updated), 0ul);
    rectangle[2] = updated;
    for (unsigned long group : {0ul, 1ul}) {
        unsigned long vertices = 0, faces = 0, perFace = 0, indices = 0;
        CHECK(Container_Mesh_GetGroup(mesh, group, &vertices, &faces, &perFace, &indices, nullptr),
              "query group bounds");
        if (!CHECK_EQ(vertices, group == 0 ? 3ul : 4ul)) continue;
        CHECK_EQ(faces, 1ul);
        CHECK_EQ(perFace, vertices);
        CHECK_EQ(indices, vertices);
        D3DRMVERTEX readback[4] = {};
        CHECK_EQ(Container_Mesh_GetVertices(mesh, group, 0, vertices, readback), 0ul);
        for (unsigned i = 0; i < vertices; ++i)
            check_vertex(readback[i], group == 0 ? tri[i] : rectangle[i]);
    }
    D3DRMBOX box = {};
    Container_Mesh_GetBox(mesh, &box.min.x);
    CHECK_NEAR(box.min.x, -2.0f, 1e-6f);
    CHECK_NEAR(box.min.y, -1.0f, 1e-6f);
    CHECK_NEAR(box.min.z, -2.0f, 1e-6f);
    CHECK_NEAR(box.max.x, 8.0f, 1e-6f);
    CHECK_NEAR(box.max.y, 3.0f, 1e-6f);
    CHECK_NEAR(box.max.z, 1.0f, 1e-6f);

    DWORD visualCount = 0;
    for (unsigned long group : {0ul, 1ul}) {
        CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, group), 0);
        Container_Mesh_HideGroup(mesh, group, 1);
        Container_Mesh_HideGroup(mesh, group, 1);
        CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, group), 1);
        CHECK_EQ(mesh->activityFrame->GetVisuals(&visualCount, nullptr), DD_OK);
        CHECK_EQ(visualCount, 1u);
        Container_Mesh_HideGroup(mesh, group, 0);
        CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, group), 0);
        CHECK_EQ(mesh->activityFrame->GetVisuals(&visualCount, nullptr), DD_OK);
        CHECK_EQ(visualCount, 2u);
    }

    unsigned long group = 1;
    IDirect3DRMMesh* second = mesh->typeData->mesh;
    CHECK(Container_Mesh_HandleSeparateMeshGroups(&second, &group), "resolve second retained group");
    CHECK_EQ(group, 0ul);
    CHECK(second != mesh->typeData->mesh, "groups own separate meshes");
    Container_Mesh_SetQuality(mesh, 1, 2);
    CHECK_EQ(second->GetGroupQuality(0), D3DRMRENDER_FLAT);
    Container_Mesh_SetQuality(mesh, 1, 3);
    CHECK_EQ(second->GetGroupQuality(0), D3DRMRENDER_GOURAUD);
    unsigned materialDestroyed = 0;
    IDirect3DRMMaterial2* material = nullptr;
    auto* rm = static_cast<IDirect3DRM3*>(s_main_d3drm);
    if (CHECK_EQ(rm->CreateMaterial(4.0f, &material), DD_OK)) {
        CHECK_EQ(material->AddDestroyCallback(destroyed, &materialDestroyed), DD_OK);
        CHECK_EQ(second->SetGroupMaterial(0, material), DD_OK);
        material->Release(); // group retains its reference
        IDirect3DRMMaterial* read = nullptr;
        if (CHECK_EQ(second->GetGroupMaterial(0, &read), DD_OK)) {
            CHECK_NEAR(read->GetPower(), 4.0f, 1e-6f);
            read->Release();
        }
    }

    unsigned long w = 0, h = 0;
    Container_Texture* texture = Container_LoadTexture2("gray.bmp", 1, &w, &h);
    unsigned textureDestroyed = 0, meshDestroyed = 0;
    if (CHECK(texture != nullptr, "load generated grayscale texture")) {
        CHECK_EQ(w, 4ul);
        CHECK_EQ(h, 4ul);
        Container_Mesh_SetTexture(mesh, 1, texture);
        IDirect3DRMTexture* bound = nullptr;
        if (CHECK_EQ(second->GetGroupTexture(0, &bound), DD_OK)) {
            CHECK(bound != nullptr, "group retains the texture");
            if (bound) {
                CHECK_EQ(bound->AddDestroyCallback(destroyed, &textureDestroyed), DD_OK);
                bound->Release();
            }
        }
    }

    Container_SetPosition(mesh, root, 2.0f, -3.0f, 5.0f);
    float source[] = {1, 2, 3}, world[3] = {}, roundTrip[3] = {};
    Container_Transform(mesh, world, source);
    CHECK_NEAR(world[0], 3.0f, 1e-6f);
    CHECK_NEAR(world[1], -1.0f, 1e-6f);
    CHECK_NEAR(world[2], 8.0f, 1e-6f);
    Container_InverseTransform(mesh, roundTrip, world);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(roundTrip[i], source[i], 1e-6f);

    CHECK_EQ(second->AddDestroyCallback(destroyed, &meshDestroyed), DD_OK);
    Container_Remove(mesh);
    CHECK_EQ(meshDestroyed, 1u);
    CHECK_EQ(materialDestroyed, 1u);
    if (texture) {
        CHECK_EQ(textureDestroyed, 0u); // owner still holds the texture
        Container_FreeTexture(texture); // group is already gone
        CHECK_EQ(textureDestroyed, 1u);
    }
}

void check_immediate(Container* root)
{
    CHECK(mesh_initialise(nullptr), "initialize immediate mesh pool");
    Container* mesh = Container_MakeMesh2(root, 2);
    if (!CHECK(mesh && mesh->typeData && mesh->typeData->transMesh, "create immediate mesh")) return;
    unsigned int faces[] = {0, 1, 2};
    if (!CHECK_EQ(Container_Mesh_AddGroup(mesh, 3, 1, 3, faces), 0ul)) {
        Container_Remove(mesh);
        return;
    }
    // Mesh.c's interop vertex is position, normal, UV, plus one 32-bit colour.
    struct Vertex { D3DRMVERTEX value; uint32_t colour; };
    static_assert(sizeof(Vertex) == 36);
    Vertex vertices[] = {
        {vertex(1, 2, 3, 0, 0), 0}, {vertex(2, 2, 3, 1, 0), 0},
        {vertex(1, 4, 3, 0, 1), 0}
    };
    CHECK_EQ(Container_Mesh_SetVertices(mesh, 0, 0, 3, vertices), 0ul);
    Container_Mesh_Scale(mesh, 2, 3, 4);
    Vertex read[3] = {};
    CHECK_EQ(Container_Mesh_GetVertices(mesh, 0, 0, 3, read), 0ul);
    for (unsigned i = 0; i < 3; ++i) {
        auto expected = vertices[i].value;
        expected.position.x *= 2;
        expected.position.y *= 3;
        expected.position.z *= 4;
        check_vertex(read[i].value, expected);
    }
    Container_Mesh_SetColourAlpha(mesh, 0, 0.2f, 0.4f, 0.6f, 0.5f);
    Container_Mesh_SetEmissive(mesh, 0, 0.1f, 0.3f, 0.5f);
    auto* immediate = static_cast<Mesh*>(mesh->typeData->transMesh);
    auto* material = mesh_get_group_material(immediate, 0);
    CHECK_NEAR(material->diffuse.r, 0.2f, 1e-6f);
    CHECK_NEAR(material->diffuse.g, 0.4f, 1e-6f);
    CHECK_NEAR(material->diffuse.b, 0.6f, 1e-6f);
    CHECK_NEAR(material->diffuse.a, 0.5f, 1e-6f);
    CHECK_NEAR(material->emissive.r, 0.1f, 1e-6f);
    CHECK_NEAR(material->emissive.g, 0.3f, 1e-6f);
    CHECK_NEAR(material->emissive.b, 0.5f, 1e-6f);
    Container_Mesh_HideGroup(mesh, 0, 1);
    CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, 0), 1);
    Container_Mesh_HideGroup(mesh, 0, 0);
    CHECK_EQ(Container_Mesh_IsGroupHidden(mesh, 0), 0);
    unsigned visualDestroyed = 0;
    CHECK_EQ(immediate->uv->AddDestroyCallback(destroyed, &visualDestroyed), DD_OK);
    Container_Remove(mesh);
    CHECK_EQ(visualDestroyed, 1u);
}

bool setup()
{
    if (!CHECK(fixture_enter("container_resource_fixture") && fixture_mkdir("Data"),
               "isolated texture directory") || !CHECK(write_texture(), "write grayscale BMP"))
        return false;
    file_initialise("ContainerResourceTest", 0, "");
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return false;
    std::strcpy(mainGlobs.programName, "ContainerResourceTest");
    mainGlobs.className = mainGlobs.programName;
    if (!CHECK(main_init_app(nullptr), "create dummy window")) return false;
    directdraw_initialise(mainGlobs.hWnd);
    DirectDraw_Driver drivers[16] = {};
    DirectDraw_Device devices[32] = {};
    unsigned int driversCount = 0, count = 0;
    if (!CHECK(directdraw_enum_drivers(drivers, &driversCount) && driversCount > 0, "enumerate drivers") ||
        !CHECK(directdraw_enum_devices(&drivers[0], devices, &count), "enumerate renderers")) return false;
    for (unsigned i = 0; i < count; ++i) {
        if (std::strstr(devices[i].desc, "Miniwin Emulation"))
            return CHECK(directdraw_setup(FALSE, nullptr, &devices[i], nullptr, 0, 0, 320, 200),
                         "start real software device");
    }
    return CHECK(false, "software device present");
}
} // namespace

int main()
{
    std::atexit(exit_guard);
    if (setup()) {
        Container* root = Container_Initialise("Generic");
        if (CHECK(root != nullptr, "create root")) {
            check_retained(root);
            check_immediate(root);
            Container_Shutdown();
        }
        directdraw_shutdown();
        DestroyWindow(static_cast<HWND>(mainGlobs.hWnd));
    }
    SDL_Quit();
    completed = true;
    return lrr_test_result();
}
