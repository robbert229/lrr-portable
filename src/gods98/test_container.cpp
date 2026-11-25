// SPDX-License-Identifier: LGPL-3.0-or-later
/* container.c — type strings, hierarchy search and reference lifetime,
 * and activity timing/cloning using neutral retained-mode frames. */
#include <miniwin/d3drm.h>
#include <fstream>
#include <filesystem>

#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/main_globs.h"
#include "test/lrr_test.h"
#include "gods98/config.h"
#include "gods98/files.h"
#include "gods98/animclone.h"
#include "gods98/lws.h"
#include "platform/native/test_fixture.h"


namespace {
void CheckHierarchy(Container* root, IDirect3DRM3* rm)
{
    Container* left = Container_Create(root);
    Container* right = Container_Create(root);
    Container* first = Container_Create(left);
    Container* second = Container_Create(right);
    if (!CHECK(left && right && first && second, "independent hierarchy")) return;
    left->masterFrame->SetName("branchA");
    right->masterFrame->SetName("branchB");
    first->masterFrame->SetName("node01");
    second->masterFrame->SetName("NODE02");

    unsigned count = 0;
    CHECK_EQ(Container_SearchTree(root, "node??", 1, &count), nullptr);
    CHECK_EQ(count, 2u);
    CHECK_EQ(Container_SearchTree(root, "NoDe??", 0, nullptr), first);
    unsigned nth = 1;
    CHECK_EQ(Container_SearchTree(root, "node??", 2, &nth), second);
    nth = 2;
    CHECK_EQ(Container_SearchTree(root, "node??", 2, &nth), nullptr);
    CHECK_EQ(Container_SearchTree(root, "node03", 0, nullptr), nullptr);
    CHECK_EQ(Container_SearchTree(left, "node02", 0, nullptr), nullptr);

    Container_SetPosition(first, left, 2.0f, 3.0f, -4.0f);
    Container_SetParent(first, right);
    CHECK_EQ(Container_GetParent(first), right);
    float pos[3] = {};
    Container_GetPosition(first, right, pos);
    CHECK_EQ(pos[0], 2.0f);
    CHECK_EQ(pos[1], 3.0f);
    CHECK_EQ(pos[2], -4.0f);
    CHECK_EQ(Container_SearchTree(left, "node??", 0, nullptr), nullptr);
    CHECK_EQ(Container_SearchTree(right, "node01", 0, nullptr), first);
    Container_SetParent(first, nullptr);
    CHECK_EQ(Container_GetParent(first), nullptr);
    CHECK_EQ(Container_SearchTree(root, "node01", 0, nullptr), nullptr);
    Container_SetParent(first, left);
    CHECK_EQ(Container_GetParent(first), left);

    // A raw descendant gets a stable, non-owning container reference.
    // Destroying the raw frame must invalidate that reference safely.
    IDirect3DRMFrame3* raw = nullptr;
    if (CHECK_EQ(rm->CreateFrame(first->masterFrame, &raw), DD_OK)) {
        raw->SetName("raw01");
        CHECK_EQ(Container_SearchOwner(raw), first);
        Container* reference = Container_SearchTree(root, "raw01", 0, nullptr);
        if (CHECK(reference != nullptr, "raw descendant reference")) {
            CHECK_EQ(reference->type, Container_Reference);
            CHECK_EQ(Container_GetMasterFrame(reference), raw);
            CHECK_EQ(Container_Frame_GetContainer(raw), reference);
            CHECK_EQ(Container_SearchOwner(raw), first);
        }
        CHECK_EQ(first->masterFrame->DeleteChild(raw), DD_OK);
        raw->Release();
        CHECK_EQ(Container_SearchTree(root, "raw01", 0, nullptr), nullptr);
        if (reference) {
            CHECK(reference->flags & 0x10, "destroy callback invalidates reference");
            Container_Remove(reference);
        }
    }
    Container_Remove(first);
    Container_Remove(second);
    Container_Remove(left);
    Container_Remove(right);
    CHECK_EQ(Container_SearchTree(root, "node??", 1, &count), nullptr);
    CHECK_EQ(count, 0u);
}

bool WriteText(const char* path, const char* text)
{
    std::ofstream out(path, std::ios::binary);
    out << text;
    out.close();
    return out.good();
}

bool PrepareActivity()
{
    if (!fixture_enter("container_activity_fixture") ||
        !fixture_mkdir("Data") || !fixture_mkdir("Data/Neutral")) return false;
    return WriteText("Data/Neutral/Neutral.ae",
        "UnitTest { Activities { Alpha A Beta B } "
        "A { FILE first LWSFILE TRUE LOOPING FALSE TRANSCOEF 0.5 TRIGGER 3 } "
        "B { FILE second LWSFILE TRUE LOOPING FALSE TRANSCOEF 0.75 } }") &&
        WriteText("Data/Neutral/first.lws",
        "LWSC\n1\n\nFirstFrame 0\nLastFrame 10\nFramesPerSecond 25\n\n"
        "AddNullObject Marker\nObjectMotion (unnamed)\n  9\n  2\n"
        "  0 0 0 0 0 0 1 1 1\n  0 0 0 0 0\n"
        "  10 0 0 0 0 0 1 1 1\n  10 0 0 0 0\n"
        "EndBehavior 1\nShadowOptions 7\n") &&
        WriteText("Data/Neutral/second.lws",
        "LWSC\n1\n\nFirstFrame 0\nLastFrame 10\nFramesPerSecond 25\n\n"
        "AddNullObject Marker\nObjectMotion (unnamed)\n  9\n  2\n"
        "  0 0 0 0 0 0 1 1 1\n  0 0 0 0 0\n"
        "  0 10 0 0 0 0 1 1 1\n  10 0 0 0 0\n"
        "EndBehavior 1\nShadowOptions 7\n");
}

void CheckMarker(Container* cont, const char* activity, float x, float y)
{
    auto* parent = Container_Frame_Find(cont, activity, 0);
    if (!CHECK(parent != nullptr, "active animation frame")) return;
    IDirect3DRMFrameArray* children = nullptr;
    if (!CHECK_EQ(parent->GetChildren(&children), DD_OK)) return;
    if (CHECK_EQ(children->GetSize(), 1u)) {
        IDirect3DRMFrame* child = nullptr;
        if (CHECK_EQ(children->GetElement(0, &child), DD_OK)) {
            IDirect3DRMFrame3* marker = nullptr;
            CHECK_EQ(child->QueryInterface(IID_IDirect3DRMFrame3, reinterpret_cast<void**>(&marker)), DD_OK);
            child->Release();
            if (marker) {
                D3DVECTOR pos = {};
                CHECK_EQ(marker->GetPosition(parent, &pos), DD_OK);
                CHECK_NEAR(pos.x, x, 1e-5);
                CHECK_NEAR(pos.y, y, 1e-5);
                marker->Release();
            }
        }
    }
    children->Release();
}

void CheckActivity(Container* root)
{
    if (!CHECK(PrepareActivity(), "write neutral activity fixture")) return;
    file_initialise("container_activity_fixture", 0, "");
    block_text_token_pool_init();
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    auto* source = Container_Load(root, "Neutral", "ACT", 0);
    block_text_token_pool_destroy();
    if (!CHECK(source != nullptr, "load generic activity")) return;
    CHECK_EQ(Container_GetActivities(source, nullptr, nullptr, nullptr), 2ul);
    CHECK_EQ(Container_IsCurrentActivity(source, "Alpha"), 1);
    CHECK_EQ(Container_IsCurrentActivity(source, "Beta"), 0);
    CHECK_EQ(Container_GetAnimationFrames(source), 10ul);
    CHECK_EQ(Container_GetTransCoef(source), 0.5f);
    CHECK_STREQ(Container_FormatPartName(source, "Marker", nullptr), "Marker_??");
    const int ordinal = 1;
    CHECK_STREQ(Container_FormatPartName(source, "Marker", &ordinal), "Marker_01");
    CHECK_EQ(Container_SetActivity(source, "Unknown"), 0);
    CHECK_EQ(Container_IsCurrentActivity(source, "Alpha"), 1);
    unsigned triggers = 0;
    Container_SetTriggerFrameCallback([](Container*, void* data) {
        ++*static_cast<unsigned*>(data);
    }, &triggers);
    CHECK_EQ(Container_SetAnimationTime(source, 2.0f), 0.0f);
    CheckMarker(source, "Alpha", 2, 0);
    CHECK_EQ(triggers, 0u);
    CHECK_EQ(Container_MoveAnimation(source, 2.0f), 0.0f);
    CheckMarker(source, "Alpha", 4, 0);
    CHECK_EQ(triggers, 1u);
    CHECK_EQ(Container_MoveAnimation(source, 1.0f), 0.0f);
    CHECK_EQ(triggers, 1u);

    // Hidden animations advance their logical time but defer the actual pose.
    Container_Hide(source, 1);
    CHECK_EQ(Container_SetAnimationTime(source, 7.0f), 0.0f);
    CHECK_EQ(Container_GetAnimationTime(source), 7.0f);
    CheckMarker(source, "Alpha", 5, 0);
    Container_ForceAnimationUpdate(source);
    CheckMarker(source, "Alpha", 7, 0);
    Container_Hide(source, 0);

    CHECK_EQ(Container_SetActivity(source, "Beta"), 1);
    CHECK_EQ(Container_IsCurrentActivity(source, "Alpha"), 0);
    CHECK_EQ(Container_IsCurrentActivity(source, "Beta"), 1);
    CHECK_EQ(Container_GetTransCoef(source), 0.75f);
    CHECK_EQ(Container_SetAnimationTime(source, 4.0f), 0.0f);
    CheckMarker(source, "Beta", 0, 4);

    auto* clone = Container_Clone(source);
    if (CHECK(clone && clone != source, "independent activity clone")) {
        CHECK_EQ(Container_GetActivities(clone, nullptr, nullptr, nullptr), 2ul);
        CHECK_EQ(Container_GetParent(clone), root);
        CHECK_EQ(Container_IsCurrentActivity(clone, "Beta"), 1);
        CHECK_EQ(Container_SetAnimationTime(clone, 2.0f), 0.0f);
        CheckMarker(clone, "Beta", 0, 2);
        CheckMarker(source, "Beta", 0, 4);
        CHECK_EQ(Container_SetActivity(clone, "Alpha"), 1);
        CHECK_EQ(Container_IsCurrentActivity(source, "Beta"), 1);
        CHECK_EQ(Container_SetAnimationTime(clone, 12.0f), 3.0f);
        CheckMarker(clone, "Alpha", 10, 0);
        CHECK_EQ(Container_GetAnimationTime(clone), 12.0f);

        Container_Remove(clone);
        CHECK_EQ(Container_GetParent(clone), nullptr);
        auto* reused = Container_Clone(source);
        CHECK_EQ(reused, clone);
        CHECK_EQ(Container_GetParent(reused), root);
        CHECK_EQ(Container_GetAnimationTime(reused), 0.0f);
        CheckMarker(reused, "Alpha", 0, 0);
        CheckMarker(source, "Beta", 0, 4);
        Container_Remove(reused);
    }
    Container_SetTriggerFrameCallback(nullptr, nullptr);
    Container_Remove(source);
}
} // namespace

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
    CheckHierarchy(root, rm3);
    CheckActivity(root);
    Container_Shutdown();
    lws_shutdown();
    s_main_d3drm = nullptr;
    rm3->Release();
    return lrr_test_result();
}
