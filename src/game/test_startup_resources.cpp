// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstdlib>
#include <cstring>

#include "gods98/config.h"
#include "gods98/directdraw.h"
#include "gods98/font_api.h"
#include "gods98/images_link_names.h"
#include "gods98/images_unit.h"
#include "game/info_unit.h"
#include "game/panel_unit.h"
#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "game/text_link_names.h"
#include "game/text.h"
#include "game/text_record.h"
#include "game/tooltip_link_names.h"
#include "game/tooltip.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

extern "C" {
void lego_panels_config_entry_apply(BlockRec*, unsigned, unsigned);
void lego_panel_button_layout_config_load(BlockRec*, unsigned, unsigned);
void lego_advisor_tutorial_icon_image_load(BlockRec*);
void lego_advisor_tooltip_and_sound_config_load(BlockRec*);
void lego_text_messages_config_load(BlockRec*);
void lego_message_load_infomessages(BlockRec*);
extern Text_Globs textGlobs;
}

using namespace synthetic_fixture;

int main()
{
    if (!CHECK(fixture_enter("startup_resources_fixture") && fixture_mkdir("Data") &&
               write_bmp("Data/image.bmp", 16, 12) &&
               write_bmp("Data/font.bmp", 80, 190), "generate isolated grayscale inputs") ||
        !initialise_headless("StartupResources")) return lrr_test_result();
    block_text_token_pool_init();
    Image_Initialise();
    legoGlobs.gameName = "Fixture";
    ui_panel_name_tables_init();
    sfx_initialise();
    info_initialise(nullptr);
    text_load(nullptr, nullptr, 0, 0, 0);
    auto* font = static_cast<TextureSurface*>(font_atlas_record_create("font.bmp"));
    if (!CHECK(font != nullptr, "load font placeholder")) return lrr_test_result();
    tooltip_initialise(font, 2, 1, 0.5f, 320, 200, 0, 0.5f, 0.5f, 0.5f);

    const char configText[] =
        "Fixture {\n"
        " Main { TutorialIcon image.bmp }\n"
        " Panels640x480 { Panel_Radar image.bmp,2,3,4,5 Panel_InfoDock missing.bmp,0,0,0,0 }\n"
        " Buttons640x480 {\n"
        "  Panel_Radar PanelButton_Radar_Toggle,image.bmp,missing.bmp,image.bmp,1,2,8,9,ToolTip_RadarToggle\n"
        "  Panel_Radar PanelButton_Radar_TaggedObjectView,NULL,image.bmp,NULL,10,11,12,13,Unknown\n"
        " }\n"
        " ToolTips { ToolTip_RadarToggle Sample_text Unknown Ignored }\n"
        " ToolTipsSFXs { ToolTip_RadarToggle SFX_NULL Unknown Unknown }\n"
        " TextMessages { Text_Walk Example_text }\n"
        " TextMessagesWithImages { Text_Drill Sample_image:image.bmp:SFX_NULL Text_NoDrill Missing_image:missing.bmp:SFX_NULL }\n"
        " InfoMessages {\n"
        "  Info_CrystalFound Sample_text:image.bmp:SFX_NULL:SLOW:ALWAYSPLAYSFX\n"
        "  Info_OreSeamFound Another_text:image.bmp:SFX_NULL:ALWAYSPLAYSFX\n"
        "  Info_Landslide Other_text:image.bmp:Unknown\n"
        "  Unknown Ignored\n"
        " }\n"
        "}";
    auto* config = block_parse(configText, sizeof(configText)-1);
    if (!CHECK(config != nullptr, "parse independent startup configuration")) return lrr_test_result();
    lego_panels_config_entry_apply(config, 320, 200);
    auto& radar = panelGlobs.panelTable[PANEL_RADAR];
    CHECK(radar.image != nullptr, "real panel image loaded");
    CHECK_EQ(radar.isFlic, 0);
    CHECK_EQ(radar.outPos.x, 2.0f);
    CHECK_EQ(radar.inPos.y, 5.0f);
    CHECK(panelGlobs.panelTable[PANEL_INFO_DOCK].image == nullptr, "missing panel stays absent");
    lego_panel_button_layout_config_load(config, 320, 200);
    if (CHECK_EQ(radar.buttonCount, 2u) && CHECK(radar.buttonList != nullptr, "button list allocated")) {
        auto& first = radar.buttonList[0];
        auto& second = radar.buttonList[1];
        CHECK(first.normalImage && first.pressedImage, "normal and pressed image loaded");
        CHECK(first.highlightImage == nullptr, "missing highlight image");
        CHECK_EQ(first.x2, 9.0f);
        CHECK_EQ(first.y2, 11.0f);
        CHECK_EQ(first.toolTip, 10);
        CHECK(second.normalImage == nullptr && second.pressedImage == nullptr, "optional images omitted");
        CHECK(second.highlightImage != nullptr, "highlight loaded");
        CHECK_EQ(second.toolTip, 0);
    }
    lego_advisor_tutorial_icon_image_load(config);
    CHECK(legoGlobs.TutorialIcon != nullptr, "tutorial icon loaded");
    lego_advisor_tooltip_and_sound_config_load(config);
    lego_text_messages_config_load(config);
    CHECK_STREQ(textGlobs.messages[Text_Walk], "Example text");
    CHECK_STREQ(textGlobs.messages[Text_Drill], "Sample image");
    CHECK(textGlobs.images[Text_Drill] != nullptr, "message image loaded");
    CHECK_STREQ(textGlobs.soundNames[Text_Drill], "SFX_NULL");
    CHECK_STREQ(textGlobs.messages[Text_NoDrill], "Missing image");
    CHECK(textGlobs.images[Text_NoDrill] == nullptr, "missing message image tolerated");
    lego_message_load_infomessages(config);
    for (int type : {0, 1, 6}) {
        CHECK(info_has_text(type), "info message configured");
        CHECK(info_get_image(type) != nullptr, "info message image loaded");
    }
    CHECK(!info_has_text(2), "unconfigured info message untouched");

    // The message replacement path releases its owned image immediately.
    text_set_message(Text_Drill, "Replacement");
    CHECK(textGlobs.images[Text_Drill] == nullptr, "replacement removes image");
    CHECK_STREQ(textGlobs.messages[Text_Drill], "Replacement");
    for (auto& message : textGlobs.messages) {
        std::free(message);
        message = nullptr;
    }
    block_close(config);
    std::free(radar.buttonList);
    radar.buttonList = nullptr;
    font_atlas_record_release(font);
    Image_Shutdown();
    directdraw_shutdown();
    return lrr_test_result();
}
