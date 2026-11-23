// SPDX-License-Identifier: LGPL-3.0-or-later
/* container.c — type strings, animation frame counts, hierarchy and
 * visibility, using retained-mode frames without a device or assets. */
#include <miniwin/d3drm.h>

#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/main_globs.h"
#include "test/lrr_test.h"

int main()
{
    CHECK_EQ(Container_GetAnimFileFrameCount("xof header DDiScene { 37; }"), 37ul);
    CHECK_EQ(Container_GetAnimFileFrameCount("DDiScene { 0; }"), 1ul);
    CHECK_EQ(Container_GetAnimFileFrameCount("xof header without a scene"), 1ul);

    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    if (!CHECK_EQ(Direct3DRMCreate(&rm), DD_OK)) return lrr_test_result();
    CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK);
    rm->Release();
    if (!rm3) return lrr_test_result();

    s_main_d3drm = rm3;
    Container* root = Container_Initialise("UnitTest");
    CHECK_EQ(Container_GetRoot(), root);
    int noTexture = 0;
    CHECK_EQ(Container_ParseTypeString("lWo:NoTexture", &noTexture), Container_LWO);
    CHECK_EQ(noTexture, 1);
    CHECK_EQ(Container_ParseTypeString("mesh", &noTexture), Container_Mesh);
    CHECK_EQ(noTexture, 0);
    CHECK_EQ(Container_ParseTypeString("unknown", &noTexture), Container_Invalid);
    CHECK_EQ(Container_ParseTypeString(nullptr, &noTexture), Container_Invalid);

    Container* child = Container_Create(root);
    if (CHECK(child != nullptr, "create child container")) {
        CHECK_EQ(Container_GetParent(child), root);
        CHECK_EQ(Container_Frame_GetOwner(Container_GetMasterFrame(child)), child);
        int value = 42;
        Container_SetUserData(child, &value);
        CHECK_EQ(Container_GetUserData(child), static_cast<void*>(&value));

        Container_SetPosition(child, root, 2.0f, -3.0f, 5.0f);
        float pos[3] = {};
        Container_GetPosition(child, root, pos);
        CHECK_NEAR(pos[0], 2.0f, 1e-6);
        CHECK_NEAR(pos[1], -3.0f, 1e-6);
        CHECK_NEAR(pos[2], 5.0f, 1e-6);

        CHECK_EQ(Container_IsHidden(child), 0);
        Container_Hide(child, 1);
        CHECK_EQ(Container_IsHidden(child), 1);
        Container_Hide(child, 1);
        Container_Hide(child, 0);
        CHECK_EQ(Container_IsHidden(child), 0);
        Container_Remove(child);
    }
    Container_Shutdown();
    s_main_d3drm = nullptr;
    rm3->Release();
    return lrr_test_result();
}
