// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_layout_fallback — Lego.cfg lays the interface out only for 640x480
 * (Panels640x480, Buttons640x480, ...). In another mode the units read
 * that layout instead of finding none ([divergence:layout-fallback],
 * docs/divergences.md), which crashed the original at 1024x768. A mode
 * with its own keys still reads them.
 *
 * Lego_LoadPanels and Lego_LoadPanelButtons read a synthetic Lego.cfg, and
 * the Panel.c calls they make reach the test's __wrap_ stubs. No game data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "gods98/config.h"
#include "game/panel_unit.h"
#include "game/lego_globs.h"

extern "C" {
void lego_panels_config_entry_apply(struct BlockRec* config, unsigned int screenWidth,
                                    unsigned int screenHeight);
void lego_panel_button_layout_config_load(struct BlockRec* config, unsigned int screenWidth,
                                          unsigned int screenHeight);
}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The Lego.cfg the units read, by name. */
static std::string s_config;

struct PanelLoad {
    std::string image;
    int panel;
    int bounds[4];
};

static std::vector<PanelLoad> s_panels;
static std::vector<unsigned int> s_buttonCounts;

extern "C" {

void* __wrap_file_load_binary(const char* filename, unsigned int* sizeptr)
{
    (void)filename;
    char* copy = (char*)malloc(s_config.size() + 1);
    memcpy(copy, s_config.c_str(), s_config.size() + 1);
    if (sizeptr) *sizeptr = (unsigned int)s_config.size();
    return copy;
}

void __wrap_legacy_overlay_image_record_load(const char* fileName, int panel, unsigned int flags)
{
    (void)flags;
    s_panels.push_back({fileName, panel, {0, 0, 0, 0}});
}

void __wrap_panel_category_bounds_set(int panel, int xOut, int yOut, int xIn, int yIn)
{
    if (!s_panels.empty() && s_panels.back().panel == panel) {
        s_panels.back().bounds[0] = xOut;
        s_panels.back().bounds[1] = yOut;
        s_panels.back().bounds[2] = xIn;
        s_panels.back().bounds[3] = yIn;
    }
}

void __wrap_panel_texture_slots_allocate(int panel, unsigned int count, const unsigned int* buttonTypes,
                                         const float* areas, char** normalNames, char** highlightNames,
                                         char** pressedNames, const int* toolTips)
{
    (void)buttonTypes; (void)areas; (void)normalNames; (void)highlightNames; (void)pressedNames;
    (void)toolTips;
    if (panel == 0) s_buttonCounts.push_back(count);
}

int __wrap_case_insensitive_name_table_lookup(const char* name, int* index)
{
    (void)name;
    *index = 0;
    return 0;
}

} /* extern "C" */

static BlockFile* LoadConfig(const char* text)
{
    s_config = text;
    return block_open("Lego.cfg");
}

static const char* kOnly640 =
    "Lego* {\n"
    "  Panels640x480 {\n"
    "    Panel_Radar  Interface\\Panels\\Radar.bmp,16,13,16,13\n"
    "  }\n"
    "  Buttons640x480 {\n"
    "    Panel_Radar  PanelButton_Radar_Toggle,NULL,NULL,NULL,1,2,3,4,NULL\n"
    "  }\n"
    "}\n";

static const char* kBoth =
    "Lego* {\n"
    "  Panels640x480 {\n"
    "    Panel_Radar  Interface\\Panels\\Radar.bmp,16,13,16,13\n"
    "  }\n"
    "  Panels1024x768 {\n"
    "    Panel_Radar  Interface\\Panels\\Radar1024.bmp,32,26,32,26\n"
    "  }\n"
    "}\n";

int main(void)
{
    block_text_token_pool_init();
    ui_panel_name_tables_init();
    legoGlobs.gameName = "Lego*";

    /* 640x480 reads its own keys, as the original. */
    BlockFile* config = LoadConfig(kOnly640);
    CHECK(config != NULL, "Lego.cfg loads");
    s_panels.clear();
    lego_panels_config_entry_apply(config, 640, 480);
    CHECK(s_panels.size() == 1 && s_panels[0].image == "Interface\\Panels\\Radar.bmp",
          "640x480 loads Panels640x480");

    /* 1024x768 has no keys of its own: it reads the 640x480 layout. */
    s_panels.clear();
    lego_panels_config_entry_apply(config, 1024, 768);
    CHECK(s_panels.size() == 1, "1024x768 loads a panel from Panels640x480");
    CHECK(!s_panels.empty() && s_panels[0].image == "Interface\\Panels\\Radar.bmp" &&
              s_panels[0].bounds[0] == 16 && s_panels[0].bounds[1] == 13,
          "1024x768 takes Panels640x480's image and position");

    s_buttonCounts.clear();
    lego_panel_button_layout_config_load(config, 1024, 768);
    CHECK(s_buttonCounts.size() == 1 && s_buttonCounts[0] == 1,
          "1024x768 loads the radar's button from Buttons640x480");
    block_close(config);

    /* A mode with its own keys keeps them. */
    config = LoadConfig(kBoth);
    s_panels.clear();
    lego_panels_config_entry_apply(config, 1024, 768);
    CHECK(s_panels.size() == 1 && s_panels[0].image == "Interface\\Panels\\Radar1024.bmp" &&
              s_panels[0].bounds[0] == 32,
          "1024x768 keeps Panels1024x768 when Lego.cfg has it");
    block_close(config);

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("layout fallback: ok\n");
    return 0;
}
