// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string>
#include <miniwin/d3drm.h>

#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/lws.h"
#include "gods98/main_globs.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/advisor.h"
#include "game/camera_control.h"
#include "game/panel_unit.h"
#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "game/text_link_names.h"
#include "game/text.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

extern "C" LegoCamera* camera_create(void*, int);
extern "C" void camera_free(LegoCamera*);

static DWORD child_count(Container* parent)
{
    IDirect3DRMFrameArray* children = nullptr;
    if (!CHECK(Container_GetMasterFrame(parent)->GetChildren(&children) == DD_OK && children,
               "get scene child list")) return 0;
    const DWORD count = children->GetSize();
    children->Release();
    return count;
}

int main(int argc, char** argv)
{
    using namespace synthetic_fixture;
    const bool reduced = argc == 2 && std::string(argv[1]) == "reduced";
    if (!CHECK(fixture_enter(reduced ? "startup_advisor_reduced" : "startup_advisor_full") &&
               fixture_mkdir("Data") && write_null_scene("Data/motion.lws"),
               "generate one generic null scene") || !initialise_headless("StartupAdvisor"))
        return lrr_test_result();
    block_text_token_pool_init();
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    auto* root = Container_Initialise("Fixture");
    if (!CHECK(root != nullptr, "create root")) return lrr_test_result();
    viewport_initialise();
    auto* camera = camera_create(root, 3);
    if (!CHECK(camera != nullptr, "create camera")) return lrr_test_result();
    auto* cameraContainer = static_cast<Container*>(camera->contCam);
    auto* view = viewport_create(0, 0, 1, 1, cameraContainer);
    if (!CHECK(view != nullptr, "create viewport")) return lrr_test_result();
    ui_panel_name_tables_init();
    sfx_initialise();
    text_load(nullptr, nullptr, 0, 0, 0);
    s_context_mode = 1;
    advisor_scene_controller_initialize("Fixture", camera, view);
    advisor_scene_visual_state_initialize(200.0f);
    const DWORD before = child_count(cameraContainer);
    constexpr unsigned reduceAnimations = 0x00400000;
    if (reduced) s_engine_feature_word |= reduceAnimations;
    std::string text =
        "Fixture {\n"
        " Advisor { Advisor_Anim_PointToMap motion,0.1,0.4 Advisor_Anim_Point_N motion,0,0.2 "
        "Advisor_Anim_Point_NE missing,0,1 }\n"
        " AdvisorPositions640x480 {\n"
        "  Advisor_Objective Advisor_Anim_PointToMap,Text_Walk,SFX_NULL,12,18,NULL\n"
        "  Advisor_ObjectiveAcheived Invalid,NULL,SFX_NULL,1,2,NULL\n"
        "  Advisor_PanelPoint_RadarToggle Advisor_Anim_Point_N,NULL,SFX_NULL,3,4,Panel_Radar\n"
        " }\n"
        "}";
    // The optional !key is only present for the reduced case, where it
    // should not be loaded. The normal fallback currently re-queries the
    // unprefixed key; it does not load optional entries.
    if (reduced) text.insert(text.find("Advisor_Anim_Point_NE"), "!Advisor_Anim_Point_E motion,0,1 ");
    auto* config = block_parse(text.c_str(), text.size());
    if (!CHECK(config != nullptr, "parse advisor config")) return lrr_test_result();
    advisor_config_slots_load_target(config, "Fixture");
    CHECK_EQ(child_count(cameraContainer), before + 2u);
    advisor_position_config_load(config, "Fixture", 320, 200);
    float x = 0, y = 0;
    advisor_action_position_get(0, &x, &y);
    CHECK_EQ(x, 12.0f);
    CHECK_EQ(y, 18.0f);
    CHECK_EQ(advisor_request_admit(1, 0), 0); // Invalid animation name was rejected.
    advisor_action_position_get(6, &x, &y);
    CHECK_EQ(x, 3.0f);
    CHECK_EQ(y, 4.0f);
    CHECK(advisor_request_admit(0, 0), "start configured advisor animation");
    CHECK(global_flag_bit0_is_set(), "animation active");
    advisor_animation_end_cleanup();
    CHECK_EQ(global_flag_bit0_is_set(), 0u);
    block_close(config);
    viewport_shutdown();
    camera_free(camera);
    Container_Shutdown();
    lws_shutdown();
    directdraw_shutdown();
    return lrr_test_result();
}
