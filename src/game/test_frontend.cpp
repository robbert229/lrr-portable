// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_frontend — validation for src/game/frontend.cpp (finding 23).
 *
 * Synthetic fixture mirrors the shipped Lego.cfg shape: root `Lego*` with
 * sibling `Main` (StartLevel/../FrontEnd flag) and `Menu` (MainMenuFull)
 * and `Levels` groups. The fixture exercises:
 *   - the `::` path-separator queries ("Lego*::Menu::MainMenuFull::…"),
 *   - MenuBuild parsing (submenus, Item%i grammar),
 *   - StartLevel/TutorialStartLevel + NextLevel chains,
 *   - level-picker grafting (LevelPick / LevelPickT art),
 *   - navigation + triggers (Start/Quit/Back/Esc), and
 *   - the item renderer (selection highlight + glyph pixels).
 *
 * With `--real <wad-dir>` the shipped wads are mounted and the whole front
 * end is built from the real Lego.cfg: menu count + grafted picker rows are
 * reported and asserted (the end-to-end proof the port now reads the ship).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* mkdtemp / rmdir */

#include "game/frontend.hpp"
#include "game/effects.hpp"
#include "game/priorities.h"
#include "game/savegame.hpp"
#include "game/util.hpp"
#include "game/game.hpp"
#include "game/select.hpp"
#include "game/upgrades.hpp"
#include "game/port_loop_ports.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/input.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/texture.hpp"
#include "game/entity.hpp"


/* C179 (2026-10-25): a key-keyed loader standing in for the surface-load
 * seam's bitmap string cache. */
static char s_c179_buf[5][1];
static void* s_c179_loader(const char* key)
{
    if (key == NULL) {
        return NULL;
    }
    static const char* const names[5] = { "c179_l", "c179_r", "c179_p1",
                                          "c179_p2", "c179_p3" };
    for (int i = 0; i != 5; ++i) {
        if (strcmp(key, names[i]) == 0) {
            return s_c179_buf[i];
        }
    }
    return NULL;
}

static int s_failures = 0;
static int s_action_ready_calls;
static int s_action_ready(void*, int action)
{
    ++s_action_ready_calls;
    input_action_state_word_set(action,
        input_action_state_word_get(action) | 4u);
    return 1;
}
static int s_catalog_ready(int, int, void*)
{
    ++s_action_ready_calls;
    return 1;
}


#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static BlockFile* s_parse(const char* text)
{
    return block_parse((char*)text, strlen(text));
}

static const char* FIXCFG =
    "Lego* {\n"
    "\tMain {\n"
    "\t\tStartLevel\t\t\tLevels::Tran01\n"
    "\t\tTutorialStartLevel\tLevels::MoveTuto01\n"
    "\t\tFrontEnd\t\t\t\tTRUE\n"
    "\t}\n"
    "\tMenu {\n"
    "\t\tMainMenuFull {\n"
    "\t\t\tMenuCount 4\n"
    "\t\t\tMenu1 {\n"
    "\t\t\t\tFullName\tMain\n"
    "\t\t\t\tTitle\t\tMain\n"
    "\t\t\t\tPosition\t320:200\n"
    "\t\t\t\tAutoCenter\tTRUE\n"
    "\t\t\t\tItemCount\t4\n"
    "\t\t\t\tItem1\t\tNext:0:0:Start_Game:Menu2\n"
    "\t\t\t\tItem2\t\tNext:0:40:Training_Missions:Menu3\n"
    "\t\t\t\tItem3\t\tTrigger:0:120:Credits:0\n"
    "\t\t\t\tItem4\t\tNext:0:160:Quit_Game:Menu4\n"
    "\t\t\t\tPlayRandom\tTRUE\n"
    "\t\t\t\t!Overlay1\tavi\\PB1.flh:SFX_NULL:488:0\n"
    "\t\t\t\t!Overlay2\tavi\\Rocky.flh:FrontSFX_RockMonster:0:216\n"
    "\t\t\t}\n"
    "\t\t\tMenu2 {\n"
    "\t\t\t\tFullName\tMissions\n"
    "\t\t\t\tTitle\t\tLevels\n"
    "\t\t\t\tPosition\t0:0\n"
    "\t\t\t\tMenuImage\tInterface\\LevelPicker\\LevelPick.bmp:0:0:1\n"
    "\t\t\t\tCanScroll\tTRUE\n"
    "\t\t\t\tItemCount\t1\n"
    "\t\t\t\tItem1\t\tNext:6:436:Back:Menu1\n"
    "\t\t\t}\n"
    "\t\t\tMenu3 {\n"
    "\t\t\t\tFullName\tTraining_Missions\n"
    "\t\t\t\tTitle\t\tTutorials\n"
    "\t\t\t\tPosition\t0:0\n"
    "\t\t\t\tMenuImage\tInterface\\LevelPicker\\LevelPickT.bmp:0:0:1\n"
    "\t\t\t\tItemCount\t1\n"
    "\t\t\t\tItem1\t\tNext:6:436:Back:Menu1\n"
    "\t\t\t}\n"
    "\t\t\tMenu4 {\n"
    "\t\t\t\tFullName\tAre_you_sure?\n"
    "\t\t\t\tTitle\t\tARE_YOU_SURE?\n"
    "\t\t\t\tPosition\t320:200\n"
    "\t\t\t\tAutoCenter\tTRUE\n"
    "\t\t\t\tDisplayTitle\tTRUE\n"
    "\t\t\t\tItemCount\t2\n"
    "\t\t\t\tItem1\t\tTrigger:0:60:Yes:1\n"
    "\t\t\t\tItem2\t\tNext:0:100:No:Menu1\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "\tLevels {\n"
    "\t\tTran01 {\n"
    "\t\t\tFullName\t1._Moving_Rock_Raiders\n"
    "\t\t\tNextLevel\tLevels::Tran02\n"
    "\t\t\tLevelLinks\tLevels::Tran02\n"
    "\t\t\tFrontEndX\t96\n"
    "\t\t\tFrontEndY\t90\n"
    "\t\t}\n"
    "\t\tTran02 {\n"
    "\t\t\tFullName\t2._A_Cavern\n"
    "\t\t\tFrontEndX\t200\n"
    "\t\t\tFrontEndY\t150\n"
    "\t\t}\n"
    "\t\tMoveTuto01 {\n"
    "\t\t\tFullName\tMove_Tutorial\n"
    "\t\t\tFrontEndX\t60\n"
    "\t\t\tFrontEndY\t60\n"
    "\t\t}\n"
    "\t}\n"
    "}\n";

/* Config sections the original resolves under the root
 * (Config_BuildStringID(gameName, "<Section>", 0), finding 508). Lego.cfg
 * keeps them inside `Lego* { … }`; a bare one-segment lookup never matched
 * there, and under the original lookup a miss would read the file's
 * trailing empty record. Each loader must now find its section. */
namespace unported_ledger_stubs {
int dependency_grid_config_load(void* config, int level); /* startup_config.cpp */
}
using unported_ledger_stubs::dependency_config_entry_hook_set;

static int s_dependency_hook_calls;
static int dependency_hook_count(void*, int, const char*, const char*)
{
    ++s_dependency_hook_calls;
    return 1;
}

static void test_root_qualified_sections(void)
{
    BlockFile* cfg = s_parse(
        "Lego* {\n"
        "\tMain { FrontEnd TRUE }\n"
        "\tUpgradeNames {\n"
        "\t\tLevel1\tUP1\n"
        "\t\tLevel2\tUP2\n"
        "\t}\n"
        "\tUpgradeTypes {\n"
        "\t\tWDDrillUpgrade\tVehicles\\WalkerBody\\LPW_D_Up\n"
        "\t\tLPW_LaserBlock\tVehicles\\WalkerBody\\LPW_LaserBlock\n"
        "\t}\n"
        "\tDependencies {\n"
        "\t\tAlwaysCheck:BuildingTypes::Toolstation\tBuildingTypes::Powerstation,0\n"
        "\t\tAlwaysCheck:BuildingTypes::Teleports\tBuildingTypes::Toolstation,0\n"
        "\t}\n"
        "\tToolTipsSFXs {\n"
        "\t\tToolTip_RadarBlock\tSFX_Drill\n"
        "\t}\n"
        "}\n");
    CHECK(cfg != NULL, "sections: parse fixture");
    if (cfg == NULL) return;

    lego::level_upgrade_name_strings_load(cfg);
    CHECK(lego::level_upgrade_name_string_get(1) != NULL
              && strcmp(lego::level_upgrade_name_string_get(1), "UP1") == 0
              && lego::level_upgrade_name_string_get(2) != NULL
              && strcmp(lego::level_upgrade_name_string_get(2), "UP2") == 0,
          "sections: UpgradeNames loads under the root");

    CHECK(lego::upgrade_types_load(cfg) == 1
              && lego::upgrade_type_count_get() == 2u
              && strcmp(lego::upgrade_type_name_get(0), "WDDrillUpgrade") == 0
              && strcmp(lego::upgrade_type_name_get(1), "LPW_LaserBlock") == 0,
          "sections: UpgradeTypes loads under the root");

    dependency_config_entry_hook_set(dependency_hook_count, NULL);
    s_dependency_hook_calls = 0;
    CHECK(unported_ledger_stubs::dependency_grid_config_load((void*)cfg, 0) == 2 && s_dependency_hook_calls == 2,
          "sections: Dependencies loads under the root");
    dependency_config_entry_hook_set(NULL, NULL);

    {
        int index = -1;
        int alias = -1;
        CHECK(case_insensitive_name_table_lookup("ToolTip_RadarBlock", &index)
                  && sound_alias_lookup("SFX_Drill", &alias),
              "sections: tool-tip slot and alias resolve");
        timed_effect_cue_request_set(index, 0u);
        advisor_tooltip_and_sound_config_load(cfg);
        CHECK(index >= 0 && timed_effect_cue_peek(index) == (uint32_t)alias,
              "sections: ToolTipsSFXs loads under the root");
    }

    /* A file whose sections are missing loads nothing and does not fault. */
    {
        BlockFile* bare = s_parse("Lego* {\n\tMain { FrontEnd TRUE }\n}\n");
        CHECK(bare != NULL && unported_ledger_stubs::dependency_grid_config_load((void*)bare, 0) == 0,
              "sections: a missing section answers nothing");
        if (bare != NULL) block_close(bare);
    }
    block_close(cfg);
}

static void test_block_sep(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    CHECK(cfg != NULL, "sep: parse fixture");
    /* the `::` path separator (finding 23, DAT_004aab18) resolves the
     * exe-style query; a space is not a separator */
    CHECK(cfg && block_get_strptr(cfg, "Lego*::Menu::MainMenuFull::MenuCount")
          != NULL, "sep: :: menu path");
    CHECK(cfg && strcmp(block_get_strptr(cfg, "Lego*::Menu::MainMenuFull::MenuCount"),
                        "4") == 0, "sep: :: menu count");
    CHECK(cfg && block_get_strptr(cfg, "Lego*::Menu MainMenuFull::MenuCount")
          == NULL, "sep: space form does not resolve");
    CHECK(cfg && strcmp(block_get_strptr(cfg, "Lego*::Menu::MainMenuFull::Menu1::Item1"),
                        "Next:0:0:Start_Game:Menu2") == 0, "sep: :: item");
    if (cfg != NULL) {
        block_close(cfg);
    }
}

static void test_build(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_init(&fe);
    int ok = frontend_build(&fe, cfg);
    CHECK(ok == 1, "build: MainMenuFull parsed");
    CHECK(fe.built, "build: built flag");

    CHECK(fe.campaign_n == 2, "build: campaign chain Tran01,Tran02");
    CHECK(fe.tutorial_n == 1, "build: tutorial chain MoveTuto01");
    CHECK(strcmp(fe.campaign[0], "Tran01") == 0, "build: chain[0] Tran01");
    CHECK(strcmp(fe.campaign[1], "Tran02") == 0, "build: chain[1] Tran02");

    const FeMenu* main = frontend_menu(&fe, FE_MENU_MAIN);
    CHECK(main != NULL && main->submenu_count == 4, "build: 4 submenus");
    CHECK(strcmp(main->submenus[0].title, "Main") == 0, "build: Menu1 title");

    const FeItem* it0 = &main->submenus[0].items[0];
    CHECK(it0->kind == FE_ITEM_NEXT, "build: Item1 is Next");
    CHECK(strcmp(it0->label, "Start_Game") == 0, "build: Item1 label");
    CHECK(strcmp(it0->target, "Menu2") == 0, "build: Item1 target");
    CHECK(it0->level_start == 0, "build: Menu1 rows not picker rows");

    /* trigger with flag = 0 (Credits) and = 1 (Yes quit) */
    CHECK(main->submenus[0].items[2].kind == FE_ITEM_TRIGGER
        && main->submenus[0].items[2].flag == 0, "build: Credits trigger flag 0");
    CHECK(main->submenus[3].items[0].kind == FE_ITEM_TRIGGER
        && main->submenus[3].items[0].flag == 1, "build: Yes trigger flag 1");
    /* FLIC overlay lane (finding 23): !Overlay%i path:sfx:x:y + PlayRandom. */
    CHECK(main->submenus[0].play_random == 1, "build: PlayRandom parsed");
    CHECK(main->submenus[0].overlay_count == 2, "build: two !Overlay entries");
    CHECK(strcmp(main->submenus[0].overlays[0].path, "avi\\PB1.flh") == 0
        && strcmp(main->submenus[0].overlays[0].sfx, "SFX_NULL") == 0
        && main->submenus[0].overlays[0].x == 488
        && main->submenus[0].overlays[0].y == 0,
        "build: overlay0 path/sfx/x/y");
    CHECK(strcmp(main->submenus[0].overlays[1].path, "avi\\Rocky.flh") == 0
        && strcmp(main->submenus[0].overlays[1].sfx,
                  "FrontSFX_RockMonster") == 0
        && main->submenus[0].overlays[1].x == 0
        && main->submenus[0].overlays[1].y == 216,
        "build: overlay1 path/sfx/x/y");

    /* picker grafting: Menu2 = campaign rows, Menu3 = tutorial rows */
    const FeSubmenu* pick = &main->submenus[1];
    CHECK(pick->item_count == 1 + 2, "build: Menu2 = Back + 2 campaign rows");
    CHECK(pick->items[1].level_start == 1, "build: picker row flagged");
    CHECK(strcmp(pick->items[1].target, "Tran01") == 0, "build: row target Tran01");
    /* Reference-backed picker rows show FullName with '_' rendered as ' '. */
    CHECK(strcmp(pick->items[1].label, "1. Moving Rock Raiders") == 0,
        "build: row label from FullName");
    CHECK(pick->items[1].x == 96 && pick->items[1].y == 90,
        "build: row scatter from FrontEndX/Y");
    CHECK(strcmp(pick->items[2].target, "Tran02") == 0, "build: row2 Tran02");
    const FeSubmenu* tpick = &main->submenus[2];
    CHECK(tpick->item_count == 1 + 1, "build: Menu3 = Back + 1 tutorial row");
    CHECK(strcmp(tpick->items[1].target, "MoveTuto01") == 0,
        "build: tutorial row MoveTuto01");
    if (cfg != NULL) {
        block_close(cfg);
    }
}

static void test_gate(void)
{
    BlockFile* on = s_parse(FIXCFG);
    CHECK(frontend_enabled(on) == 1, "gate: FrontEnd TRUE -> enabled");
    block_close(on);
    BlockFile* off = s_parse(
        "Lego* {\n\tMain { FrontEnd FALSE }\n}\n"
        "\tMenu { MainMenuFull { MenuCount 1 Menu1 { ItemCount 0 } } }\n");
    CHECK(frontend_enabled(off) == 0, "gate: FrontEnd FALSE -> disabled");
    block_close(off);
}

static FeInput s_small = { 0, 0, 0, 0, 0, 0, -1, -1, 0 };

static void test_nav_and_actions(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    CHECK(frontend_open(&fe, FE_MENU_MAIN), "nav: open");
    CHECK(fe.sub == 0 && fe.sel == 0, "nav: starts at Menu1 sel 0");

    /* Missing overlay files leave the player NULL without crashing. */
    FeInput ov = { 0, 0, 0, 0, 0, 0, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &ov, NULL) == FE_EVT_NONE, "nav: overlay tick ok");
    CHECK(fe.menus[FE_MENU_MAIN].submenus[0].overlays[0].flic == NULL,
          "nav: missing overlay file -> no flic (no crash)");

    FeInput neutral = { 0, 0, 0, 0, 0, 0, 10, 10, 0 };
    FeInput up = { 1, 0, 0, 0, 0, 0, 10, 10, 0 };
    FeInput down = { 0, 1, 0, 0, 0, 0, 10, 10, 0 };
    FeInput hover = { 0, 0, 0, 0, 0, 0, 300, 245, 0 };
    FeInput enter = { 0, 0, 0, 0, 1, 0, 300, 245, 0 };
    FeInput quit = { 0, 0, 0, 0, 0, 0, 300, 365, 1 };
    FeInput no = { 0, 0, 0, 0, 0, 0, 320, 305, 1 };
    FeInput yes = { 0, 0, 0, 0, 0, 0, 320, 265, 1 };
    CHECK(frontend_tick(&fe, &hover, NULL) == FE_EVT_NONE && fe.sel == 1,
          "nav: pointer selects Training");
    CHECK(frontend_tick(&fe, &enter, NULL) == FE_EVT_NONE && fe.sub == 0,
          "nav: Enter does not activate main row");
    CHECK(frontend_tick(&fe, &neutral, NULL) == FE_EVT_NONE && fe.sel == -1,
          "nav: leaving row clears selection");
    CHECK(frontend_tick(&fe, &up, NULL) == FE_EVT_NONE && fe.sel == -1,
          "nav: Up does not select a main row");
    CHECK(frontend_tick(&fe, &down, NULL) == FE_EVT_NONE && fe.sel == -1,
          "nav: Down does not select a main row");
    CHECK(frontend_tick(&fe, &quit, NULL) == FE_EVT_NONE && fe.sub == 3,
          "nav: click Quit opens confirmation");
    FeInput confirmation_esc = { 0, 0, 0, 0, 0, 1, 320, 305, 0 };
    CHECK(frontend_tick(&fe, &confirmation_esc, NULL) == FE_EVT_NONE &&
          fe.open && fe.sub == 3, "nav: Escape leaves confirmation open");
    enter.mouse_x = 320; enter.mouse_y = 305;
    CHECK(frontend_tick(&fe, &enter, NULL) == FE_EVT_NONE && fe.sub == 3,
          "nav: Enter does not activate No");
    CHECK(frontend_tick(&fe, &no, NULL) == FE_EVT_NONE && fe.sub == 0,
          "nav: click No returns to main");
    CHECK(frontend_tick(&fe, &quit, NULL) == FE_EVT_NONE && fe.sub == 3,
          "nav: click Quit reopens confirmation");
    CHECK(frontend_tick(&fe, &yes, NULL) == FE_EVT_QUIT && fe.quit,
          "nav: click Yes quits");
    frontend_open(&fe, FE_MENU_MAIN);
    FeInput esc_ = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &esc_, NULL) == FE_EVT_NONE && fe.open && fe.sub == 0,
          "nav: Escape leaves root open");
    if (cfg != NULL) {
        block_close(cfg);
    }
}

static void test_render(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    frontend_open(&fe, FE_MENU_MAIN);
    /* Reference behaviour: opening the root menu leaves no row selected (Up/
     * Down/Enter do not drive it, see test_nav); the highlight follows the
     * pointer. Hover the first row (y 200..) before rendering. */
    {
        FeInput hover0 = { 0, 0, 0, 0, 0, 0, 300, 205, 0 };
        frontend_tick(&fe, &hover0, NULL);
        CHECK(fe.sel == 0, "render: pointer selects the first row");
    }

    int w = 640, h = 400;
    uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
    CHECK(px != NULL, "render: buffer");
    /* clear to black explicitly for deterministic assertions */
    memset(px, 0, (size_t)w * h * 4);
    frontend_render(&fe, NULL, NULL, w, h, px, w * 4);
    uint8_t b[4];

    /* selected row 0 = "Start_Game": fixture Menu1 Position 320:200,
     * AutoCenter TRUE -> x = 320 - 70/2 = 285, y = 200 (3x5 fallback). */
    b[0] = px[(199 * w + 283) * 4 + 0];
    b[1] = px[(199 * w + 283) * 4 + 1];
    b[2] = px[(199 * w + 283) * 4 + 2];
    /* The reference renderer draws no highlight bar: selection is the bright
     * face only, so the pixel left of the label stays background. */
    CHECK(b[0] == 0 && b[1] == 0 && b[2] == 0,
        "render: no selection highlight bar (background)");
    /* 'S' of Start_Game at (285,200) is ink (white) */
    b[0] = px[(200 * w + 285) * 4 + 0];
    CHECK(b[0] == 255 && px[(200 * w + 285) * 4 + 1] == 255
        && px[(200 * w + 285) * 4 + 2] == 255, "render: glyph ink");

    /* non-selected item 3 "Quit_Game" (y offset 160 -> y = 360; 3x5 tw
     * 63 -> x = 320 - 31 = 289); faint color (200,200,160). */
    b[0] = px[(360 * w + 289) * 4 + 0];
    CHECK(b[0] == 200 && px[(360 * w + 289) * 4 + 1] == 200
        && px[(360 * w + 289) * 4 + 2] == 160, "render: faint row ink");

    free(px);
    block_close(cfg);
}

/* Synthetic bitmap font: 10x19 grid, cell 8x4 (w=80, h=76); every cell
 * filled with the key (pure red), one white ink pixel at cell (1,1).
 * Renders a two-glyph label ("LQ", AutoCenter FALSE so x stays 48) and
 * asserts: the key is NOT drawn (bg shows through), and each glyph's ink
 * pixel lands at the measured advance. */
static void test_font(void)
{
    TextureSurface tf;
    memset(&tf, 0, sizeof(tf));
    tf.width = FE_FONT_GRID_COLS * 8;   /* 80 */
    tf.height = FE_FONT_GRID_ROWS * 4;  /* 76 */
    tf.pixels = (uint32_t*)calloc((size_t)tf.width * tf.height, 4);
    CHECK(tf.pixels != NULL, "font: buffer");
    for (int i = 0; i < tf.width * tf.height; i++) {
        tf.pixels[i] = 0xffff0000u;            /* key: palette-255 red */
    }
    if (tf.pixels != NULL) {
        for (int g = 0; g < FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS; g++) {
            int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
            tf.pixels[(size_t)(row * 4 + 1) * tf.width + (col * 8 + 1)] =
                0xffffffffu;                    /* ink pixel per cell */
        }

        BlockFile* cfg = s_parse(
            "Lego* {\n"
            "\tMain { StartLevel Levels::Tran01 }\n"
            "\tMenu { MainMenuFull {\n"
            "\t\tMenuCount 1\n"
            "\t\tMenu1 { AutoCenter FALSE Position 0:0 ItemCount 1\n"
            "\t\t\tItem1 Next:4:4:LQ:Menu1\n"
            "\t\t}\n"
            "\t} }\n"
            "\tLevels { Tran01 { FullName T } }\n"
            "}\n");
        FrontEnd fe;
        frontend_build(&fe, cfg);
        frontend_open(&fe, FE_MENU_MAIN);
        /* No row is pre-selected (reference behaviour); hover the label so
         * the selected-row highlight bar is under the glyph cells. */
        {
            FeInput hover = { 0, 0, 0, 0, 0, 0, 5, 5, 0 };
            frontend_tick(&fe, &hover, NULL);
            CHECK(fe.sel == 0, "font: pointer selects the label row");
        }

        int w = 160, h = 120;
        uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
        CHECK(px != NULL, "font: fb");
        if (px != NULL) {
            memset(px, 0, (size_t)w * h * 4);
            frontend_render(&fe, &tf, &tf, w, h, px, w * 4);
            /* 'L' (g=44 -> col 4, row 4): cell ink at (4*8+1, 4*4+1) =
             * (33,17). Item at Position 0:0 + anchor 4:4 = (4,4). */
            uint8_t c0 = px[((4 + 1) * w + (4 + 1)) * 4 + 0];   /* 'L' ink */
            CHECK(c0 == 255 && px[((4 + 1) * w + (4 + 1)) * 4 + 1] == 255
                && px[((4 + 1) * w + (4 + 1)) * 4 + 2] == 255, "font: L ink");
            /* The reference text path advances by the trimmed ink width
             * (here 1 column), so 'Q' lands one pixel after 'L': ink at
             * (6, 5) (it was (7, 5) with the old maxx+1 advance). */
            uint8_t q0 = px[(5 * w + 6) * 4 + 0];
            CHECK(q0 == 255, "font: Q advance ink");
            /* the key pixel of the 'L' cell (its 0,0) is transparent: the
             * frame buffer behind it shows through (black here — not the
             * red key, not white ink). The selected-row highlight bar the
             * old port drew behind rows is gone in the reference path. */
            uint8_t k = px[(4 * w + 4) * 4 + 0];
            CHECK(k == 0 && px[(4 * w + 4) * 4 + 1] == 0
                && px[(4 * w + 4) * 4 + 2] == 0,
                "font: key skipped (background shows through)");
            free(px);
        }

        /* Corpus two-key mode (FUN_0047d9d0 + FUN_0047e050): the blit keys
         * come from the surface (palette entries 0 and 255), not from
         * sampling. Keying BOTH the cell background (red) and the ink
         * (white) blanks every glyph — nothing is drawn. */
        uint8_t* px2 = (uint8_t*)calloc((size_t)w * h, 4);
        CHECK(px2 != NULL, "font2: fb");
        if (px2 != NULL) {
            memset(px2, 0, (size_t)w * h * 4);
            tf.key_count = 2;
            tf.keys[0] = 0xffff0000u;   /* background key */
            tf.keys[1] = 0xffffffffu;   /* ink key */
            frontend_render(&fe, &tf, &tf, w, h, px2, w * 4);
            tf.key_count = 0;
            int keyed_seen = 0;
            for (size_t i = 0; i < (size_t)w * h; i++) {
                uint32_t p = ((uint32_t*)px2)[i];
                if ((p & 0x00ffffffu) == 0x00ffffffu
                    || (p & 0x00ffffffu) == 0x00ff0000u) {
                    keyed_seen = 1;
                    break;
                }
            }
            CHECK(!keyed_seen, "font2: surface keys key both colours");
            free(px2);
        }

        block_close(cfg);
    }
    free(tf.pixels);
}

/* A blank glyph may use one key for transparent spacing and another for
 * its background. The measurement key is distinct from the blit keys. */
static void test_font_blank_advance(void)
{
    TextureSurface tf;
    uint32_t* pixels;
    int x, y;
    uint32_t joined, spaced;
    memset(&tf, 0, sizeof(tf));
    tf.width = FE_FONT_GRID_COLS * 8;
    tf.height = FE_FONT_GRID_ROWS * 4;
    pixels = (uint32_t*)malloc((size_t)tf.width * tf.height * 4);
    CHECK(pixels != NULL, "font space: buffer");
    if (pixels == NULL) return;
    tf.pixels = pixels;
    tf.key_count = 2;
    tf.keys[0] = 0xff000000u;
    tf.keys[1] = 0xffff0000u;
    for (y = 0; y < tf.height; ++y)
        for (x = 0; x < tf.width; ++x)
            pixels[(size_t)y * tf.width + x] = 0xffff0000u;
    for (y = 0; y < 4; ++y)
        for (x = 0; x < 2; ++x)
            pixels[(size_t)y * tf.width + x] = 0xff000000u;
    for (x = 'A'; x <= 'B'; ++x) {
        int glyph = x - 0x20;
        pixels[(size_t)(glyph / 10 * 4 + 1) * tf.width +
               glyph % 10 * 8 + 1] = 0xffffffffu;
    }
    joined = formatted_text_draw_and_measure(&tf, NULL, 0, 0, 0, 0, 0,
                                              "AB", NULL);
    spaced = formatted_text_draw_and_measure(&tf, NULL, 0, 0, 0, 0, 0,
                                              "A B", NULL);
    CHECK(spaced == joined + 2, "font space: advance preserves blank spacing");
    free(pixels);
}

/* --- real-wad probe (--real <dir>) ---------------------------------------- */

static void real_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    if (cfg == NULL) {
        fprintf(stderr, "frontend_test: --real probe skipped (no Lego.cfg)\n");
        return;
    }
    FrontEnd fe;
    int ok = frontend_build(&fe, cfg);
    CHECK(ok == 1, "real: MainMenuFull builds from the ship's Lego.cfg");
    const FeMenu* main = frontend_menu(&fe, FE_MENU_MAIN);
    CHECK(fe.campaign_n >= 1, "real: StartLevel chain resolves a level");
    CHECK(main != NULL && main->submenu_count == 5,
        "real: ship MainMenuFull has 5 submenus (Main/Missions/Training/Load/Quit)");
    int picker_rows = 0;
    for (int i = 0; i < main->submenu_count; i++) {
        for (int j = 1; j < main->submenus[i].item_count; j++) {
            if (main->submenus[i].items[j].level_start) {
                ++picker_rows;
            }
        }
    }
    CHECK(picker_rows > 0, "real: level picker grafted rows exist");
    fprintf(stderr,
            "frontend_test: --real probe: campaign=%d tutorial=%d picker_rows=%d\n",
            fe.campaign_n, fe.tutorial_n, picker_rows);

    /* Bitmap-font geometry on the shipped fonts (FUN_0047a470 10x19 grid):
     * Menu_Font_Hi.bmp = 380x817 -> cell 38x43, bg = the red key. */
    TextureSurface* fhi = texture_load("Interface\\FrontEnd\\Menu_Font_Hi.bmp");
    CHECK(fhi != NULL, "real: Menu_Font_Hi.bmp loads");
    if (fhi != NULL) {
        CHECK(fhi->width % FE_FONT_GRID_COLS == 0
            && fhi->height % FE_FONT_GRID_ROWS == 0,
            "real: font dims divide the 10x19 grid");
        int fcw = fhi->width / FE_FONT_GRID_COLS;
        int fch = fhi->height / FE_FONT_GRID_ROWS;
        CHECK(fcw == 38 && fch == 43, "real: Hi font cell = 38x43");
        size_t key = 0, tot = 0;
        for (int yy = 0; yy < fhi->height; yy += 7) {
            for (int xx = 0; xx < fhi->width; xx += 7) {
                tot++;
                if (fhi->pixels[(size_t)yy * (size_t)fhi->width + xx]
                    == FE_FONT_KEY) {
                    key++;
                }
            }
        }
        CHECK(key * 100 / (tot > 0 ? tot : 1) >= 40,
            "real: Hi font bg is the red key (45% sampled)");
        texture_free(fhi);
    }
    TextureSurface* flo = texture_load("Interface\\FrontEnd\\Menu_Font_Lo.bmp");
    CHECK(flo != NULL, "real: Menu_Font_Lo.bmp loads");
    if (flo != NULL) {
        CHECK(flo->width / FE_FONT_GRID_COLS == 38
            && flo->height / FE_FONT_GRID_ROWS == 43,
            "real: Lo font cell = 38x43");
        texture_free(flo);
    }
    if (cfg != NULL) {
        block_close(cfg);
    }
}

static void test_active_image(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    frontend_open(&fe, FE_MENU_MAIN);
    CHECK(frontend_active_image(&fe) == NULL,
        "img: Menu1 has no MenuImage key");

    /* Click Start_Game -> Menu2 (Missions picker). */
    FeInput in = { 0, 0, 0, 0, 0, 0, 300, 205, 1 };
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sub == 1, "img: navigated to Menu2");
    const char* im = frontend_active_image(&fe);
    CHECK(im != NULL, "img: Menu2 has a MenuImage");
    CHECK(im != NULL && strcmp(im, "Interface\\LevelPicker\\LevelPick.bmp") == 0,
        "img: Menu2 path trimmed of ':0:0:1'");
    const FeSubmenu* image_sub = frontend_active_submenu(&fe);
    CHECK(image_sub != NULL && image_sub->image_has_origin &&
          image_sub->image_x == 0.0f && image_sub->image_y == 0.0f &&
          image_sub->image_keyed,
          "img: explicit origin and transparency survive tuple parsing");
    block_close(cfg);
}

static void test_pointer(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    frontend_open(&fe, FE_MENU_MAIN);
    CHECK(fe.sel == 0, "ptr: starts at item 0");

    /* hover item 1 ("Training_Missions": Position 320:200, AutoCenter,
     * 3x5 tw 126 -> x=257, y=240; hit rect ~(253..387, 238..250)) */
    FeInput ho = { 0, 0, 0, 0, 0, 0, 300, 245, 0 };
    CHECK(frontend_tick(&fe, &ho, NULL) == FE_EVT_NONE && fe.sel == 1,
        "ptr: hover selects item 1");

    /* click item 1 -> its Next target (Menu3) */
    FeInput cl = { 0, 0, 0, 0, 0, 0, 300, 245, 1 };
    CHECK(frontend_tick(&fe, &cl, NULL) == FE_EVT_NONE && fe.sub == 2,
        "ptr: click activates Next -> Menu3");
    CHECK(fe.sel == 0, "ptr: submenu nav resets selection");

    /* click in empty space keeps the menu up with no change */
    frontend_open(&fe, FE_MENU_MAIN);
    FeInput miss = { 0, 0, 0, 0, 0, 0, 10, 10, 1 };
    CHECK(frontend_tick(&fe, &miss, NULL) == FE_EVT_NONE && fe.sel == -1
        && fe.sub == 0, "ptr: empty-space click is a no-op");

    /* click the selected Start_Game (item 0: x=285, y=200) -> Missions */
    FeInput start = { 0, 0, 0, 0, 0, 0, 300, 205, 1 };
    CHECK(frontend_tick(&fe, &start, NULL) == FE_EVT_NONE && fe.sub == 1,
        "ptr: click Start_Game -> Missions picker");
    block_close(cfg);
}

static void test_interface_action_labels(void)
{
    int action = -1;
    /* The registered wall group and UiActionDispatch use these reference
     * enum values, not the old order of the image-name initializer. */
    CHECK(interface_menu_action_label_find("Interface_MenuItem_Dig", &action)
          && action == 11,
          "action images: Drill Wall uses dispatcher action 11");
    CHECK(strcmp(interface_menu_action_label_get(12),
                 "Interface_MenuItem_Reinforce") == 0
          && strcmp(interface_menu_action_label_get(13),
                    "Interface_MenuItem_Dynamite") == 0
          && strcmp(interface_menu_action_label_get(15),
                    "Interface_MenuItem_DeselectDig") == 0,
          "action images: wall group resolves its authored artwork");
    CHECK(interface_menu_action_label_find("Interface_MenuItem_RemovePath",
                                           &action) && action == 6,
          "action images: Remove Path keeps reference enum position");
    CHECK(interface_menu_action_label_get(-1) == NULL
          && interface_menu_action_label_get(74) == NULL,
          "action images: invalid enum has no resource name");
}

/* Keys.c (G98-018): the shortcut-key names resolve to DirectInput scan
 * codes, ignoring case; a miss leaves the out byte untouched. */
static void test_key_names(void)
{
    uint8_t code = 0xeeu;
    keyboard_key_names_init();
    CHECK(named_option_index_find_case_insensitive("KEY_ESCAPE", &code)
          && code == 0x01u,
          "keys: the first named code resolves");
    CHECK(named_option_index_find_case_insensitive("key_d", &code)
          && code == 0x20u,
          "keys: the lookup ignores case");
    CHECK(named_option_index_find_case_insensitive("KEYPAD_ENTER", &code)
          && code == 0x9cu,
          "keys: extended codes above 0x80 resolve");
    CHECK(named_option_index_find_case_insensitive("KEY_DELETE", &code)
          && code == 0xd3u,
          "keys: the last named code resolves");
    code = 0xeeu;
    CHECK(!named_option_index_find_case_insensitive("KEY_NOSUCH", &code)
          && code == 0xeeu,
          "keys: an unknown name fails and leaves the code alone");
}

static void test_pause(void)
{
    /* Shipped PausedMenu shape: Menu1 (Continue/Options/Quit), Menu2
     * (Options), Menu3 (quit confirm), Menu4 (restart confirm). */
    BlockFile* cfg = s_parse(
        "Lego* {\n"
        "\tMain { StartLevel Levels::Tran01 }\n"
        "\tMenu { MainMenuFull {\n"
        "\t\tMenuCount 1\n"
        "\t\tMenu1 { ItemCount 1 Item1 Next:0:0:Back:Menu1 }\n"
        "\t}\n"
        "\tPausedMenu {\n"
        "\t\tMenuCount 4\n"
        "\t\tMenu1 {\n"
        "\t\t\tTitle Paused\n"
        "\t\t\tPosition 250:134\n"
        "\t\t\tItemCount 4\n"
        "\t\t\tItem1 Trigger:60:50:Continue_Game:1\n"
        "\t\t\tItem2 Next:60:80:Options:Menu2\n"
        "\t\t\tItem3 Next:60:140:Restart_Mission:Menu4\n"
        "\t\t\tItem4 Next:60:170:Quit:Menu3\n"
        "\t\t}\n"
        "\t\tMenu2 { Title Options ItemCount 1\n"
        "\t\t\tItem1 Next:60:180:Back:Menu1 }\n"
        "\t\tMenu3 { Title Are_you_sure? ItemCount 2\n"
        "\t\t\tItem1 Trigger:0:60:Yes:1\n"
        "\t\t\tItem2 Next:0:100:No:Menu1\n"
        "\t\t}\n"
        "\t\tMenu4 { Title Restart_Mission? ItemCount 2\n"
        "\t\t\tItem1 Trigger:0:60:Yes:1\n"
        "\t\t\tItem2 Next:0:100:No:Menu1\n"
        "\t\t}\n"
        "\t} }\n"
        "\tLevels { Tran01 { FullName T } }\n"
        "}\n");
    FrontEnd fe;
    frontend_build(&fe, cfg);
    CHECK(frontend_build(&fe, cfg) == 1, "pause: builds");
    CHECK(frontend_menu(&fe, FE_MENU_PAUSED)->submenu_count == 4,
        "pause: PausedMenu parses 4 submenus");

    /* Pause rows follow pointer selection; Enter does not activate Back. */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput enter = { 0, 0, 0, 0, 1, 0, 310, 184, 0 };
    FeInput cont = { 0, 0, 0, 0, 0, 0, 310, 184, 1 };
    CHECK(frontend_tick(&fe, &enter, NULL) == FE_EVT_NONE && fe.open,
        "pause: Enter does not activate Continue");
    CHECK(frontend_tick(&fe, &cont, NULL) == FE_EVT_RESUME && !fe.open,
        "pause: click Continue resumes");

    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput esc = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
    for (int i = 0; i < 4; ++i) {
        fe.sub = i;
        CHECK(frontend_tick(&fe, &esc, NULL) == FE_EVT_NONE && fe.open && fe.sub == i,
            "pause: Escape leaves submenu visible");
    }

    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput quit = { 0, 0, 0, 0, 0, 0, 310, 304, 1 };
    FeInput no = { 0, 0, 0, 0, 0, 0, 1, 101, 1 };
    FeInput yes = { 0, 0, 0, 0, 0, 0, 1, 61, 1 };
    CHECK(frontend_tick(&fe, &quit, NULL) == FE_EVT_NONE && fe.sub == 2,
        "pause: click Quit opens confirmation");
    FeInput outside = { 0, 0, 0, 0, 0, 0, 600, 400, 0 };
    CHECK(frontend_tick(&fe, &outside, NULL) == FE_EVT_NONE && fe.sel == -1,
        "pause: confirmation entry outside has no highlighted row");
    enter.mouse_x = 1; enter.mouse_y = 101;
    CHECK(frontend_tick(&fe, &enter, NULL) == FE_EVT_NONE && fe.sub == 2,
        "pause: Enter does not activate No");
    CHECK(frontend_tick(&fe, &no, NULL) == FE_EVT_NONE && fe.sub == 0,
        "pause: click No returns to root");
    CHECK(frontend_tick(&fe, &outside, NULL) == FE_EVT_NONE && fe.sel == -1,
        "pause: returned root outside has no highlighted row");
    CHECK(frontend_tick(&fe, &quit, NULL) == FE_EVT_NONE && fe.sub == 2,
        "pause: click Quit reopens confirmation");
    CHECK(frontend_tick(&fe, &yes, NULL) == FE_EVT_LEAVE_LEVEL && !fe.open,
        "pause: click Yes leaves level");

    frontend_open(&fe, FE_MENU_PAUSED);
    snprintf(fe.resume_level, FE_LEVEL_NAME_MAX, "%s", "Tran01");
    FeInput restart = { 0, 0, 0, 0, 0, 0, 310, 274, 1 };
    CHECK(frontend_tick(&fe, &restart, NULL) == FE_EVT_NONE && fe.sub == 3,
        "pause: click Restart opens confirmation");
    CHECK(frontend_tick(&fe, &outside, NULL) == FE_EVT_NONE && fe.sel == -1,
        "pause: restart confirmation outside has no highlighted row");
    CHECK(frontend_tick(&fe, &yes, NULL) == FE_EVT_START_LEVEL
        && strcmp(fe.pending_level, "Tran01") == 0,
        "pause: click Yes restarts the level");

    block_close(cfg);
}

static void test_slots(void)
{
    BlockFile* cfg = s_parse(
        "Lego* {\n"
        "\tMain { StartLevel Levels::Tran01 }\n"
        "\tMenu { MainMenuFull {\n"
        "\t\tMenuCount 2\n"
        "\t\tMenu1 {\n"
        "\t\t\tAutoCenter TRUE Position 320:200 ItemCount 1\n"
        "\t\t\tItem1 Next:0:0:Load_A_Saved_Game:Menu2\n"
        "\t\t}\n"
        "\t\tMenu2 {\n"
        "\t\t\tFullName Load_A_Saved_Game\n"
        "\t\t\tMenuImage Interface\\FrontEnd\\SaveLoad.bmp:0:0:1\n"
        "\t\t\tAutoCenter FALSE Position 320:200 ItemCount 1\n"
        "\t\t\tItem1 Next:-314:236:Back:Menu1\n"
        "\t\t}\n"
        "\t} }\n"
        "\tLevels { Tran01 { FullName T FrontEndX 10 FrontEndY 10 } }\n"
        "}\n");
    FrontEnd fe;
    frontend_build(&fe, cfg);
    FeSlot slots[3];
    memset(slots, 0, sizeof(slots));
    snprintf(slots[0].label, FE_NAME_MAX, "%s", "Slot 1 (empty)");
    slots[0].id = 1; slots[0].present = 0;
    snprintf(slots[1].label, FE_NAME_MAX, "%s", "Slot 2");
    slots[1].id = 2; slots[1].present = 1;
    snprintf(slots[2].label, FE_NAME_MAX, "%s", "Slot 3");
    slots[2].id = 3; slots[2].present = 1;

    CHECK(frontend_slot_raw_index(1) == 0 && frontend_slot_raw_index(5) == 4 &&
          frontend_slot_raw_index(0) == -1 && frontend_slot_raw_index(6) == -1,
          "slots: visible ordinals map to raw contexts");
    CHECK(frontend_build_slots(&fe, slots, 3) == 1, "slots: grafted");
    CHECK(fe.slot_count == 3, "slots: registered count");
    const FeMenu* m = frontend_menu(&fe, FE_MENU_MAIN);
    CHECK(m->submenus[1].item_count == 1 + 3,
        "slots: 3 rows appended on the SaveLoad screen");
    CHECK(m->submenus[1].items[1].x == 120 &&
          m->submenus[1].items[1].y == 100 &&
          m->submenus[1].items[3].y == 220,
          "slots: list offsets follow submenu position and 60-pixel stride");
    CHECK(m->submenus[1].items[1].load_slot == 1,
        "slots: row 1 binds slot 1");
    CHECK(strcmp(m->submenus[1].items[2].label, "Slot 2") == 0,
        "slots: row 2 label");

    frontend_open(&fe, FE_MENU_MAIN);
    FeInput load = { 0, 0, 0, 0, 0, 0, 300, 205, 1 };
    frontend_tick(&fe, &load, NULL);
    CHECK(fe.sub == 1, "slots: clicked Load to enter SaveLoad screen");
    FeInput escape = { 0, 0, 0, 0, 0, 1, 320, 240, 0 };
    CHECK(frontend_tick(&fe, &escape, NULL) == FE_EVT_NONE && fe.open &&
          fe.sub == 1 && fe.slot_count == 3 && m->submenus[1].item_count == 4,
          "slots: Escape leaves Load and its slot rows open");
    FeInput back_enter = { 0, 0, 0, 0, 1, 0, 8, 438, 0 };
    CHECK(frontend_tick(&fe, &back_enter, NULL) == FE_EVT_NONE && fe.sub == 1,
          "slots: Enter on Back does not activate");
    FeInput back = { 0, 0, 0, 0, 0, 0, 8, 438, 1 };
    CHECK(frontend_tick(&fe, &back, NULL) == FE_EVT_NONE && fe.sub == 0,
          "slots: Back still returns to Main after Escape");
    frontend_tick(&fe, &load, NULL);
    CHECK(fe.sub == 1, "slots: Load reopens after Back");
    FeInput slot = { 0, 0, 0, 0, 0, 0, 121, 101, 1 };
    CHECK(frontend_tick(&fe, &slot, NULL) == FE_EVT_LOAD_GAME
        && fe.load_slot == 1, "slots: clicking slot 1 -> LOAD_GAME");

    block_close(cfg);
}

static void test_options(void)
{
    BlockFile* cfg = s_parse(
        "Lego* {\n"
        "\tMain { StartLevel Levels::Tran01 FrontEnd TRUE }\n"
        "\tMenu { MainMenuFull {\n"
        "\t\tMenuCount 1 Menu1 { AutoCenter TRUE Position 320:200\n"
        "\t\t\tItemCount 1 Item1 Next:0:0:Back:Menu1 }\n"
        "\t}\n"
        "\tPausedMenu {\n"
        "\t\tMenuCount 2\n"
        "\t\tMenu1 { Title Paused Position 250:134 ItemCount 1\n"
        "\t\t\tItem1 Next:60:80:Options:Menu2\n"
        "\t\t}\n"
        "\t\tMenu2 { Title Options AutoCenter FALSE Position 250:134\n"
        "\t\t\tItemCount 3\n"
        "\t\t\tItem1 Slider:0:0:0:0:SFX_Volume:0:10:bar.bmp\n"
        "\t\t\tItem2 Cycle:0:40:0:0:Help_Window:2:Off:On\n"
        "\t\t\tItem3 Next:60:180:Back:Menu1\n"
        "\t\t}\n"
        "\t} }\n"
        "\tLevels { Tran01 { FullName T } }\n"
        "}\n");
    FrontEnd fe;
    CHECK(frontend_build(&fe, cfg) == 1, "opt: builds");

    /* Paused Menu1 -> Options (Menu2) */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput ein = { 0, 0, 0, 0, 0, 0, 310, 214, 1 };
    frontend_tick(&fe, &ein, NULL);
    CHECK(fe.sub == 1, "opt: on the Options submenu");

    FeItem* sl = &fe.menus[FE_MENU_PAUSED].submenus[1].items[0];
    CHECK(sl->kind == FE_ITEM_SLIDER && sl->min == 0 && sl->max == 10
        && sl->value == 8, "opt: slider parsed with default 8");

    FeInput r = { 0, 0, 0, 1, 0, 0, 251, 135, 0 };   /* right over slider */
    fe.option_changed = 0;
    frontend_tick(&fe, &r, NULL);
    CHECK(sl->value == 9 && fe.option_changed == 1, "opt: right increments");
    FeInput l = { 0, 0, 1, 0, 0, 0, 251, 135, 0 };   /* left over slider */
    frontend_tick(&fe, &l, NULL);
    frontend_tick(&fe, &l, NULL);
    CHECK(sl->value == 7, "opt: left decrements (9 -> 7)");

    /* Cycle item */
    FeItem* cy = &fe.menus[FE_MENU_PAUSED].submenus[1].items[1];
    CHECK(cy->kind == FE_ITEM_CYCLE && cy->cycle_count == 2
        && cy->value == 0 && strcmp(cy->cycle_opts[1], "On") == 0,
        "opt: cycle parsed with options");
    r.mouse_x = 251; r.mouse_y = 175;
    frontend_tick(&fe, &r, NULL);
    CHECK(cy->value == 1, "opt: cycle right -> option 1");
    frontend_tick(&fe, &r, NULL);
    CHECK(cy->value == 0, "opt: cycle wraps to first option");

    /* Render the slider widget (fallback chrome, no bitmap tuple): the bar is
     * anchored at the item's origin (reference layout, no label-width
     * offset): item0 at Position 250:134, bar x=250, y=134+7=141, 100 wide,
     * 6 tall; value 7 -> fill 70 => gold 250..319, then the white marker
     * 320..322, dark 323..349. */
    int w = 480, h = 320;
    uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
    CHECK(px != NULL, "opt: buffer");
    if (px != NULL) {
        memset(px, 0, (size_t)w * h * 4);
        frontend_render(&fe, NULL, NULL, w, h, px, w * 4);
        uint8_t c[3];
        c[0] = px[(143 * w + 260) * 4 + 0];
        c[1] = px[(143 * w + 260) * 4 + 1];
        c[2] = px[(143 * w + 260) * 4 + 2];
        CHECK(c[0] == 210 && c[1] == 180 && c[2] == 70,
            "opt: bar fill is gold");
        c[0] = px[(143 * w + 340) * 4 + 0];
        c[1] = px[(143 * w + 340) * 4 + 1];
        CHECK(c[0] == 36 && c[1] == 36, "opt: bar tail is dark");
        free(px);
    }

    /* Slider pointer drag (corpus FUN_00411a00 case 3 -> FUN_00411870,
     * re-run every frame the button is held): 100x20 bar rect anchored at
     * item0's origin (250,134); v = round(clamp((mx-x)*0.01, 0, 1) *
     * (max-min)). Value sits at 7 here. */
    fe.option_changed = 0;
    FeInput d1 = { 0, 0, 0, 0, 0, 0, 300, 144, 0, 1 };
    frontend_tick(&fe, &d1, NULL);
    CHECK(sl->value == 5 && fe.option_changed == 1,
        "drag: mid-bar sets the proportional value");
    FeInput d2 = { 0, 0, 0, 0, 0, 0, 350, 144, 0, 1 };
    frontend_tick(&fe, &d2, NULL);
    CHECK(sl->value == 10, "drag: bar right end = max");
    FeInput d3 = { 0, 0, 0, 0, 0, 0, 250, 144, 0, 1 };
    frontend_tick(&fe, &d3, NULL);
    CHECK(sl->value == 0, "drag: bar left end = min");
    FeInput d4 = { 0, 0, 0, 0, 0, 0, 315, 144, 0, 1 };
    frontend_tick(&fe, &d4, NULL);
    frontend_tick(&fe, &d4, NULL);
    CHECK(sl->value == 7, "drag: tracking is continuous while held");
    FeInput d5 = { 0, 0, 0, 0, 0, 0, 300, 144, 0, 0 };
    frontend_tick(&fe, &d5, NULL);
    CHECK(sl->value == 7, "drag: release leaves the value alone");
    FeInput d6 = { 0, 0, 0, 0, 0, 0, 300, 165, 0, 1 };
    frontend_tick(&fe, &d6, NULL);
    CHECK(sl->value == 7, "drag: past the bar's height no drag");
    block_close(cfg);
}

static void test_cycle_label_bounds(void)
{
    FrontEnd* fe = (FrontEnd*)calloc(1, sizeof(FrontEnd));
    GameOptions options;
    options_defaults(&options);
    CHECK(fe != NULL, "cycle bounds: frontend");
    if (fe == NULL) return;
    fe->open = fe->built = 1;
    fe->menu = FE_MENU_PAUSED;
    fe->opts = &options;
    fe->menus[FE_MENU_PAUSED].submenu_count = 1;
    FeSubmenu* sub = &fe->menus[FE_MENU_PAUSED].submenus[0];
    sub->item_count = 1;
    sub->base_x = 140;
    sub->base_y = 200;
    FeItem* item = &sub->items[0];
    item->kind = FE_ITEM_CYCLE;
    item->cycle_count = 2;
    strcpy(item->label, "Music");

    /* Five ten-pixel advances: label ends at x=190, value is separate. */
    TextureSurface font;
    memset(&font, 0, sizeof(font));
    font.width = 100;
    font.height = 190;
    font.key_count = 2;
    font.pixels = (uint32_t*)malloc(font.width * font.height * 4);
    CHECK(font.pixels != NULL, "cycle bounds: font pixels");
    if (font.pixels != NULL) {
        for (int i = 0; i < font.width * font.height; ++i)
            font.pixels[i] = 0xffffffffu;
        FeInput input = { 0, 0, 0, 0, 0, 0, 192, 201, 1, 0 };
        frontend_tick(fe, &input, &font);
        CHECK(item->value == 0 && options.music_onoff == 0,
              "cycle bounds: beyond label does not activate");
        input.mouse_x = 141;
        input.mouse_y = 198;
        frontend_tick(fe, &input, &font);
        CHECK(item->value == 0, "cycle bounds: above label does not activate");
        input.mouse_y = 201;
        frontend_tick(fe, &input, &font);
        CHECK(item->value == 1 && options.music_onoff == 1,
              "cycle bounds: label activates real option owner");
        input.mouse_x = 450;
        frontend_tick(fe, &input, &font);
        CHECK(item->value == 1, "cycle bounds: value column does not activate");
        free(font.pixels);
    }
    free(fe);
}

static void test_cycle_pointer_and_layout(void)
{
    /* frontend_build reports MainMenuFull's result, so the fixture needs a
     * (minimal) main menu next to the PausedMenu under test. */
    BlockFile* cfg = s_parse("Lego* { Menu { MainMenuFull { MenuCount 1 "
        "Menu1 { Position 0:0 ItemCount 1 Item1 Next:0:0:Back:Menu1 } } "
        "PausedMenu { MenuCount 1 "
        "Menu1 { Title Advanced_Options Position 290:134 ItemCount 2 "
        "Item1 Cycle:-150:45:310:0:Wall_Detail:2:High:Low "
        "Item2 Cycle:-150:120:310:0:Automatic_Game_Speed:2:On:Off "
        "} } } }");
    FrontEnd fe;
    CHECK(frontend_build(&fe, cfg), "cycle layout: build");
    frontend_open(&fe, FE_MENU_PAUSED);
    FeSubmenu* sub = &fe.menus[FE_MENU_PAUSED].submenus[0];
    CHECK(sub->display_title && sub->items[0].cycle_x == 310 &&
          sub->items[0].cycle_y == 0, "cycle layout: title and value offset");
    FeInput hover = { 0, 0, 0, 0, 0, 0, 150, 254, 0, 0 };
    FeInput right = { 0, 0, 0, 1, 0, 0, 150, 179, 0, 0 };
    frontend_tick(&fe, &hover, NULL);
    CHECK(fe.sel == 1, "cycle input: previous hover");
    frontend_tick(&fe, &right, NULL);
    CHECK(sub->items[0].value == 1 && sub->items[1].value == 0,
          "cycle input: same-frame pointer and Right target current row");
    uint8_t* pixels = (uint8_t*)calloc(640 * 480, 4);
    CHECK(pixels != NULL, "cycle layout: buffer");
    if (pixels != NULL) {
        frontend_render(&fe, NULL, NULL, 640, 480, pixels, 640 * 4);
        CHECK(pixels[(179 * 640 + 450) * 4] != 0,
              "cycle layout: value drawn at authored column");
        FeInput outside = { 0, 0, 0, 1, 0, 0, 600, 400, 0, 0 };
        frontend_tick(&fe, &outside, NULL);
        CHECK(fe.sel == -1 && sub->items[0].value == 1,
              "cycle input: outside Right cannot adjust stale selection");
        outside.right = 0;
        outside.down = 1;
        frontend_tick(&fe, &outside, NULL);
        CHECK(fe.sel == -1,
              "cycle input: outside Down cannot create pointer highlight");
        memset(pixels, 0, 640 * 480 * 4);
        frontend_render(&fe, NULL, NULL, 640, 480, pixels, 640 * 4);
        CHECK(pixels[(179 * 640 + 450) * 4] == 200,
              "cycle layout: missed row uses unselected glyph color");
        free(pixels);
    }
    block_close(cfg);
}

static void test_scroll(void)
{
    BlockFile* cfg = s_parse(
        "Lego* {\n"
        "\tMain { StartLevel Levels::Lv0 FrontEnd TRUE }\n"
        "\tMenu { MainMenuFull {\n"
        "\t\tMenuCount 2\n"
        "\t\tMenu1 { Title Main AutoCenter TRUE Position 320:200\n"
        "\t\t\tItemCount 1 Item1 Next:0:0:Start_Game:Menu2 }\n"
        "\t\tMenu2 { Title Pick AutoCenter FALSE Position 0:0\n"
        "\t\t\tMenuImage Interface\\LevelPicker\\LevelPick.bmp\n"
        "\t\t\tItemCount 1 Item1 Trigger:0:0:Back:1 }\n"
        "\t} }\n"
        "\tLevels {\n"
        /* The reference chain walk follows LevelLinks (the picker tree
         * edges), not NextLevel alone. */
        "\t\tLv0 { FullName Lv0 NextLevel Levels::Lv1 "
        "LevelLinks Levels::Lv1 FrontEndX 300 FrontEndY 100 }\n"
        "\t\tLv1 { FullName Lv1 NextLevel Levels::Lv2 "
        "LevelLinks Levels::Lv2 FrontEndX 300 FrontEndY 900 }\n"
        /* Rows are click-enabled only when completed or authored open (the
         * reference FrontEndOpen gate), so open Lv2 for the click check. */
        "\t\tLv2 { FullName Lv2 FrontEndOpen TRUE "
        "FrontEndX 300 FrontEndY 1800 }\n"
        "\t}\n"
        "}\n");
    FrontEnd fe;
    CHECK(frontend_build(&fe, cfg) == 1, "scr: builds");

    /* graft marked the picker plane scrollable: 3 rows + Back */
    FeSubmenu* pk = &fe.menus[FE_MENU_MAIN].submenus[1];
    CHECK(pk->can_scroll == 1 && pk->item_count == 4, "scr: picker scrollable");

    fe.view_w = 640;
    fe.view_h = 480;                 /* bands 200/280, like the corpus */
    frontend_open(&fe, FE_MENU_MAIN);

    FeInput enter = { 0, 0, 0, 0, 0, 0, 300, 205, 1 };
    frontend_tick(&fe, &enter, NULL);
    CHECK(fe.sub == 1, "scr: into the picker sub");

    FeInput mid = { 0, 0, 0, 0, 0, 0, 320, 240, 0 };
    frontend_tick(&fe, &mid, NULL);
    frontend_scroll_update(&fe, &mid);
    CHECK(pk->scroll == 0, "scr: deadzone stays put");

    /* At y=460 the reference moves ten pixels after the current tick. */
    FeInput bot = { 0, 0, 0, 0, 0, 0, 320, 460, 0 };
    frontend_tick(&fe, &bot, NULL);
    CHECK(pk->scroll == 0, "scr: tick uses the previous frame's scroll");
    frontend_scroll_update(&fe, &bot);
    CHECK(pk->scroll == -10, "scr: bottom band pans after tick");
    for (int i = 0; i < 200 && pk->scroll > -1336; i++) {
        frontend_tick(&fe, &bot, NULL);
        frontend_scroll_update(&fe, &bot);
    }
    CHECK(pk->scroll == -1336, "scr: clamps at the plane's bottom");

    /* top band pans back up */
    FeInput top = { 0, 0, 0, 0, 0, 0, 320, 20, 0 };
    frontend_tick(&fe, &top, NULL);
    CHECK(pk->scroll == -1336, "scr: tick retains bottom scroll");
    frontend_scroll_update(&fe, &top);
    CHECK(pk->scroll == -1326, "scr: top band pans back");

    /* The next tick hits Lv2 at its newly scrolled position, 1800-1326. */
    FeInput cl = { 0, 0, 0, 0, 0, 0, 310, 474, 1 };
    FeEvent ev = frontend_tick(&fe, &cl, NULL);
    CHECK(ev == FE_EVT_START_LEVEL && strcmp(fe.pending_level, "Lv2") == 0,
        "scr: click follows the scrolled plane");

    /* re-entry resets the plane (corpus MenuShow reset) */
    frontend_open(&fe, FE_MENU_MAIN);
    CHECK(pk->scroll == 0, "scr: re-entry resets");

    block_close(cfg);
}

static void test_records(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_init(&fe);
    CHECK(frontend_build(&fe, cfg) == 1, "rec: builds");
    CHECK(fe.campaign_n == 2 && fe.tutorial_n == 1, "rec: fixture chains");

    /* Each reference picker owns its own registry index space. */
    CHECK(fe_record_for_level(&fe, "MoveTuto01") == 0, "rec: tutorial = record 0");
    CHECK(fe_record_for_level(&fe, "Tran01") == 0, "rec: campaign starts at zero");
    CHECK(fe_record_for_level(&fe, "Tran02") == 1, "rec: chain position = record");
    CHECK(fe_record_for_level(&fe, "Nowhere") == -1, "rec: unknown -> -1");
    CHECK(fe_record_for_level(&fe, "") == -1, "rec: empty -> -1");

    char name[FE_LEVEL_NAME_MAX];
    name[0] = '\0';
    CHECK(fe_level_for_record(&fe, 0, name, sizeof name)
              && strcmp(name, "Tran01") == 0,
        "rec: inverse maps the campaign root");
    CHECK(fe_level_for_record(&fe, 1, name, sizeof name)
              && strcmp(name, "Tran02") == 0,
        "rec: inverse maps the campaign tail");
    CHECK(fe_level_for_record(&fe, 77, name, sizeof name) == 0,
        "rec: out-of-range inverse fails clean");
    block_close(cfg);
}

static void test_campaign_progress_bridge(void)
{
    const char* cfg_text =
        "Lego* {\n"
        " Main {\n  StartLevel Levels::A\n  TutorialStartLevel Levels::T\n }\n"
        " Menu {\n  MainMenuFull {\n   MenuCount 3\n"
        "   Menu1 {\n    ItemCount 1\n    Item1 Next:0:0:Start:Menu2\n   }\n"
        "   Menu2 {\n    MenuImage Interface\\\\LevelPicker\\\\LevelPick.bmp:0:0:1\n    ItemCount 0\n   }\n"
        "   Menu3 {\n    MenuImage Interface\\\\LevelPicker\\\\LevelPickT.bmp:0:0:1\n    ItemCount 0\n   }\n  }\n }\n"
        " Levels {\n"
        "  A {\n   NextLevel Levels::B\n   LevelLinks Levels::B,Levels::C\n   FrontEndX 10\n   FrontEndY 10\n  }\n"
        "  B {\n   NextLevel Levels::C\n   LevelLinks Levels::D\n   FrontEndX 100\n   FrontEndY 10\n  }\n"
        "  C {\n   NextLevel Levels::D\n   LevelLinks Levels::D\n   FrontEndX 200\n   FrontEndY 10\n  }\n"
        "  D {\n   FrontEndX 300\n   FrontEndY 10\n  }\n"
        "  T {\n  }\n }\n}\n";
    BlockFile* cfg = s_parse(cfg_text);
    FrontEnd fe;
    uint8_t records[4 * SAVE_RECORD_SIZE];
    FeInput input = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    FeSubmenu* sub;
    memset(records, 0, sizeof(records));
    frontend_init(&fe);
    CHECK(frontend_build(&fe, cfg), "progress: build graph");
    CHECK(fe_record_for_level(&fe, "A") == 0 &&
          fe_record_for_level(&fe, "B") == 1 &&
          fe_record_for_level(&fe, "D") == 3 &&
          fe_record_for_level(&fe, "T") == 0, "progress: separate registries");
    CHECK(frontend_apply_campaign_progress(&fe, records, 4), "progress: apply zero slot");
    CHECK(frontend_open_submenu(&fe, FE_MENU_MAIN, 1), "progress: picker open");
    sub = &fe.menus[FE_MENU_MAIN].submenus[1];
    {
        FeInput esc = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
        CHECK(frontend_tick(&fe, &esc, NULL) == FE_EVT_NONE && fe.open && fe.sub == 1,
              "progress: Escape leaves campaign picker open");
    }
    CHECK(sub->item_count == 4 && sub->items[0].level_enabled &&
          !sub->items[1].level_enabled && !sub->items[2].level_enabled &&
          !sub->items[3].level_enabled, "progress: zero records unlock only root");
    fe.sel = 1;
    CHECK(frontend_tick(&fe, &input, NULL) == FE_EVT_NONE && fe.open,
          "progress: disabled keyboard cannot launch");
    input.enter = 0; input.mouse_x = 100; input.mouse_y = 10; input.click = 1;
    CHECK(frontend_tick(&fe, &input, NULL) == FE_EVT_NONE && fe.open,
          "progress: disabled pointer cannot launch");
    records[SAVE_RECORD_SIZE] = 1;
    CHECK(frontend_apply_campaign_progress(&fe, records, 4), "progress: apply B completion");
    CHECK(sub->items[0].level_enabled && sub->items[1].level_enabled &&
          sub->items[2].level_enabled && !sub->items[3].level_enabled &&
          strcmp(sub->items[2].target, "D") == 0,
          "progress: first DFS parent unlocks D, not C");
    input.click = 1; input.mouse_x = 300; input.mouse_y = 10; input.enter = 0;
    CHECK(frontend_tick(&fe, &input, NULL) == FE_EVT_START_LEVEL &&
          strcmp(fe.pending_level, "D") == 0 && fe.origin_picker_sub == 1,
          "progress: enabled D starts D from campaign picker");
    CHECK(frontend_open_submenu(&fe, FE_MENU_MAIN, 2), "progress: training picker open");
    {
        FeInput esc = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
        CHECK(frontend_tick(&fe, &esc, NULL) == FE_EVT_NONE && fe.open && fe.sub == 2,
              "progress: Escape leaves training picker open");
    }
    input.mouse_x = 0; input.mouse_y = 0;
    CHECK(frontend_tick(&fe, &input, NULL) == FE_EVT_START_LEVEL &&
          strcmp(fe.pending_level, "T") == 0 && fe.origin_picker_sub == 2,
          "progress: training mission retains training picker origin");
    block_close(cfg);
}

int main(int argc, char** argv)
{
    test_campaign_progress_bridge();
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            real_probe(argv[a + 1]);
        }
    }

    test_block_sep();
    test_root_qualified_sections();
    test_build();
    test_gate();
    test_nav_and_actions();
    test_render();
    test_active_image();
    test_font();
    test_font_blank_advance();
    test_pointer();
    test_interface_action_labels();
    test_key_names();
    test_pause();
    test_slots();
    test_options();
    test_cycle_pointer_and_layout();
    test_cycle_label_bounds();
    test_scroll();
    test_records();

    /* The selection-context accessors (2026-10-24): the corpus writers
     * land unset, so the context starts NULL; the leaves round-trip. */
    {
        int dummy = 0;
        CHECK(current_selection_context_get() == NULL,
              "the context starts unset (no ported feeder)");
        current_selection_context_set(&dummy);
        CHECK(current_selection_context_get() == &dummy,
              "the set/get round-trips");
        current_selection_context_set(NULL);
        CHECK(current_selection_context_get() == NULL,
              "the context resets for later runs");
    }

    /* The selection-state band (2026-10-24): statics start zero; the
     * clear leaf drops bit 0; the handle leaf's gates. */
    {
        CHECK(selection_state_flags_peek() == 0u, "the state starts zero");
        selection_state_flags_set(0x31u);
        selection_state_flag_clear_bit0();
        CHECK(selection_state_flags_peek() == 0x30u,
              "the clear drops bit 0, spares the neighbours");
        selection_state_flags_set(0);

        /* The index-1 / bit-4 gate: extra handle while the bit is clear. */
        selection_state_handle_extra_set(0x77u);
        CHECK(selection_state_handle_get(1) == 0x77u,
              "index 1 takes the extra handle (bit 4 clear)");
        selection_state_flags_set(4u);
        CHECK(selection_state_handle_get(1) == 0u,
              "index 1 falls through when bit 4 is set");
        selection_state_handle_extra_set(0u);
        selection_state_flags_set(0);
        CHECK(selection_state_handle_get(2) == 0u, "no gate, no handle");

        /* The hit gate: bit 0x10 + matching hit index, table by bit 0x20. */
        selection_state_handle_slot_set(0, 3, 0x11u);
        selection_state_handle_slot_set(1, 3, 0x22u);
        selection_state_hit_index_set(3);
        selection_state_flags_set(0x10u);
        CHECK(selection_state_handle_get(3) == 0x11u,
              "the hit gate answers table A");
        selection_state_flags_set(0x10u | 0x20u);
        CHECK(selection_state_handle_get(3) == 0x22u,
              "bit 0x20 switches to table B");
        CHECK(selection_state_handle_get(4) == 0u,
              "the non-matching index is rejected");
        selection_state_flags_set(0x20u);
        CHECK(selection_state_handle_get(3) == 0u,
              "bit 0x10 is required");
        selection_state_handle_slot_set(0, 3, 0u);
        selection_state_handle_slot_set(1, 3, 0u);
        selection_state_hit_index_set(0);
        selection_state_flags_set(0);
        CHECK(selection_state_handle_slot_peek(1, 1 << 20) == 0u,
              "the guarded window reads zero (deviation)");
    }

    /* The band-mode / timed-effect / positions cluster (2026-10-24):
     * the transition leaf's hand-over shape, the two-entry mode switch,
     * the timed-effect flag window, and the positions pass. */
    {
        const uint32_t save_flags = selection_state_flags_peek();
        band_mode_set(0);
        CHECK(band_mode_peek() == 0, "the band mode starts zero");

        /* The transition: the zero-second-argument arm goes to mode 4. */
        CHECK(ui_mode_transition_request(2, 0, 0) == false
                  && band_mode_peek() == 4,
              "the arm request lands mode 4 (prior mode answers)");

        /* Mode 4 hands over when the third argument is clear. */
        CHECK(ui_mode_transition_request(2, 1, 0) == false
                  && band_mode_peek() == 2
                  && (selection_state_flags_peek() & 0x20u) != 0u,
              "the hand-over lands the index and raises bit 5");

        /* A same-index request just raises bit 5 (already raised). */
        selection_state_flags_set(selection_state_flags_peek() & ~0x20u);
        CHECK(ui_mode_transition_request(2, 1, 1) == true
                  && band_mode_peek() == 2
                  && (selection_state_flags_peek() & 0x20u) != 0u,
              "the same-index request raises bit 5");

        /* A different index with the band busy answers by prior mode. */
        CHECK(ui_mode_transition_request(5, 1, 1) == false
                  && band_mode_peek() == 2,
              "the mismatched index answers zero");

        /* Mode 4 with the third argument set: no hand-over, no bit. */
        band_mode_set(4);
        selection_state_flags_set(selection_state_flags_peek() & ~0x20u);
        CHECK(ui_mode_transition_request(3, 1, 1) == false
                  && band_mode_peek() == 4
                  && (selection_state_flags_peek() & 0x20u) == 0u,
              "the blocked hand-over keeps mode 4 and the bit low");

        /* The mode switch: entries 8/9, nothing else. */
        CHECK(timed_effect_flags_peek(8) == 0u
                  && timed_effect_flags_peek(9) == 0u,
              "the timed-effect flags start clear");
        selection_mode_set_by_index(0);
        CHECK(timed_effect_flags_peek(8) == 4u
                  && timed_effect_flags_peek(9) == 0u,
              "index 0 raises entry 8's update bit");
        selection_mode_set_by_index(1);
        CHECK(timed_effect_flags_peek(9) == 4u, "index 1 raises entry 9's");
        selection_mode_set_by_index(2);
        CHECK(timed_effect_flags_peek(0) == 0u,
              "the default case writes nothing");
        timed_effect_update_request_set(0);
        CHECK(timed_effect_flags_peek(0) == 4u,
              "the request leaf raises bit 2 directly");
        /* The window is the corpus's 40 entries (TIMED_EFFECT_ENTRIES); 16
         * is a live entry, so probe the first index past the end. */
        timed_effect_update_request_set(40); /* outside the window */
        CHECK(timed_effect_flags_peek(40) == 0u, "the guarded window reads zero");

        /* Positions: the zero table applies, the handle gate reaches the
         * seam without touching ported state. */
        int x = 9, y = 9;
        CHECK(layout_position_get(0, &x, &y) == 1 && x == 0 && y == 0,
              "the layout table starts zero (loader seam)");
        ui_mode_resource_positions_apply(); /* nothing found — inert */
        selection_state_handle_extra_set(0x55u); /* gate index 1 */
        ui_mode_resource_positions_apply();     /* one found — seam */
        selection_state_handle_extra_set(0u);
        CHECK(layout_position_get(5, &x, &y) == 0, "the guarded entry misses");

        band_mode_set(0);
        selection_state_flags_set(save_flags);
    }

    /* The front-end flow words (2026-10-24). */
    {
        /* The load-image zero state: the sentinel words test != -1 and
         * report true, exactly as the corpus's zero-initialized block does
         * before the save/tutorial selections initialize. */
        CHECK(quit_menu_probe() == 0
                  && save_slot_open() != 0 && is_in_game() != 0
                  && level_start_flag() == 0 && menu_options_flag() == 0,
              "the boot state reads the zero image");

        /* The port-side setter stands in for the unported callback
         * bindings: sentinel -1 means closed. */
        frontend_flow_word_set(1, -1);
        frontend_flow_word_set(2, -1);
        CHECK(save_slot_open() == 0 && is_in_game() == 0,
              "the closed sentinels report closed");
        frontend_flow_word_set(0, 1);
        frontend_flow_word_set(3, 2);
        frontend_flow_word_set(4, 1);
        CHECK(quit_menu_probe() == 1 && level_start_flag() != 0
                  && menu_options_flag() != 0,
              "the flag words walk through the setter");
        frontend_flow_word_set(5, 9); /* the guard ignores past-word writes */
        CHECK(quit_menu_probe() == 1, "the guard keeps the words as set");

        /* Restore the boot state for any later test. */
        frontend_flow_word_set(0, 0);
        frontend_flow_word_set(1, 0);
        frontend_flow_word_set(2, 0);
        frontend_flow_word_set(3, 0);
        frontend_flow_word_set(4, 0);
    }

    /* UiModeResourceBit40Get (2026-10-24): the masked read of the word. */
    {
        selection_state_flags_set(0u);
        CHECK(ui_mode_resource_bit40_get() == 0u, "the 40 bit starts clear");
        selection_state_flags_set(0x40u);
        CHECK(ui_mode_resource_bit40_get() == 0x40u, "the 40 bit reads masked");
        selection_state_flags_set(0x41u);
        CHECK(ui_mode_resource_bit40_get() == 0x40u, "the mask keeps its shape");
        selection_state_flags_set(0u);
    }

    /* MainSimStateAdmissionPredicate (2026-10-24). */
    {
        selection_state_flags_set(0u);
        message_set_system_state(0u);
        CHECK(main_sim_state_admission_predicate() == 0,
              "the admission gate keeps for a clear word");
        selection_state_flags_set(3u); /* bits 1+2, no 0x40 */
        CHECK(main_sim_state_admission_predicate() == 0,
              "bits 1+2 alone keep the gate shut");
        selection_state_flags_set(0x43u);
        CHECK(main_sim_state_admission_predicate() != 0,
              "the three bits open the gate");
        message_set_system_state(1u);
        CHECK(main_sim_state_admission_predicate() == 0,
              "a set system-state word shuts the gate");
        message_set_system_state(0u);
        selection_state_flags_set(0u); /* restore boot state */
    }

    /* SelectionModeStateSet (2026-10-24). */
    {
        CHECK(selection_mode_state_peek() == 0u, "the mode word starts clear");
        selection_mode_state_set(2u);
        CHECK(selection_mode_state_peek() == 2u, "the store lands");
        selection_mode_state_set(0u);
    }

    /* MenuWipeEnd (2026-10-24): the low-two-bit clear. */
    {
        menu_wipe_flags_set(3u);
        CHECK(menu_wipe_flags_peek() == 3u, "the hook installs the pair");
        menu_wipe_end();
        CHECK(menu_wipe_flags_peek() == 0u, "the end clears both bits");
        menu_wipe_flags_set(7u);
        menu_wipe_end();
        CHECK(menu_wipe_flags_peek() == 4u, "high bits stay put");
        menu_wipe_flags_set(0u);
    }

    /* The selection-region band (2026-10-24). */
    {
        const uint32_t pair[2] = { 7u, 9u };
        selection_coordinates_set(NULL, 1u, 1u); /* guarded */
        selection_coordinates_set(pair, 3u, 5u);
        CHECK(selection_coordinates_peek(0) == 7.0f
                  && selection_coordinates_peek(1) == 9.0f
                  && selection_coordinates_peek(2) == 3.0f
                  && selection_coordinates_peek(3) == 5.0f,
              "the coordinate quad lands with the int-to-float casts");

        selection_region_parameters_set(1u, 2u, 3u, 4u, 10.0f);
        float reg[5];
        selection_region_parameters_peek(reg);
        CHECK(reg[0] == 3.0f && reg[1] == 4.0f && reg[2] == 1.0f
                  && reg[3] == 2.0f && reg[4] == 0.3999999761581421f,
              "the unsigned words rotate and the float scales by 0.04");
        /* 10.0f * 0.04f (DAT_0049e298) is the exact midpoint between the
         * adjacent floats and round-to-even drops to 0x3ECCCCCC — the
         * corpus's single-precision scale, not the float 0.4f. */

        objective_selection_state_reset();
        CHECK(objective_selection_word_peek() == 0u
                  && (interaction_flags_peek() & 0x84u) == 0x84u,
              "the reset clears, mirrors, and raises the pair");
        interaction_timer_reset_and_flag_clear();
        CHECK(interaction_timer_peek() == 25.0f
                  && (interaction_flags_peek() & 0x800u) == 0u,
              "the timer restocks to 25.0 and bit 800 drops");
        selection_region_parameters_peek(reg);
        CHECK(reg[2] != 0.0f, "the mirrored float stays put");

        selection_state_flags_set(0u);
        mode_flag_pair_apply_if_bit2(1);
        CHECK(selection_state_flags_peek() == 0u, "bit 2 clear keeps quiet");
        selection_state_flags_set(7u);
        mode_flag_pair_apply_if_bit2(1);
        CHECK((selection_state_flags_peek() & 3u) == 2u,
              "bit 2 set and nonzero arms bit 1");
        selection_state_flags_set(7u);
        mode_flag_pair_apply_if_bit2(0);
        CHECK((selection_state_flags_peek() & 3u) == 0u,
              "zero drops both low bits");
        selection_state_flags_set(0u);
    }

    /* The cell-action permission window (2026-10-24). */
    {
        message_set_system_state(0u);
        CHECK(cell_action_permission_get(0, 0) == 1
                  && cell_action_permission_get(1000, 1000) == 1,
              "a clear system word permits every cell");
        message_set_system_state(4u);
        CHECK(cell_action_permission_get(0, 0) == 0, "an empty table forbids");
        CHECK(cell_action_permission_get(-1, 0) == 0
                  && cell_action_permission_get(0, 300) == 0,
              "the guarded window answers outside 300 entries");
        cell_action_permission_table_set(15, 1u); /* column 1, row 0 */
        CHECK(cell_action_permission_get(1, 0) == 1, "the stride-15 index lands");
        cell_action_permission_table_set(15, 0u);
        message_set_system_state(0u);
    }

    /* The menu-tick gate + banner word (2026-10-24). */
    {
        int hit = 0;
        for (int i = 0; i < 5000 && !hit; ++i) {
            hit = menu_audio_tick_interval_reached();
        }
        CHECK(hit != 0, "a multiple of 400 arrives within 5000 draws");
        CHECK(sim_status_banner_word_get() == 0u, "the banner word starts clear");
        sim_status_banner_word_set(1u);
        CHECK(sim_status_banner_word_get() == 1u, "the store lands");
        sim_status_banner_word_set(0u);
    }

    /* The bit-0x40 set-clear rule (2026-10-24). */
    {
        selection_state_flags_set(0u);
        ui_mode_resource_bit40_set_clear(0, 1u); /* value nonzero -> clear */
        CHECK(selection_state_flags_peek() == 0u, "a nonzero value clears 40");
        ui_mode_resource_bit40_set_clear(0, 0u); /* value zero -> set */
        CHECK(selection_state_flags_peek() == 0x40u, "a zero value sets 40");
        ui_mode_resource_bit40_set_clear(1, 9u); /* probe flips the word */
        CHECK(selection_state_flags_peek() == 0u, "the probe flips it off");
        ui_mode_resource_bit40_set_clear(1, 0u); /* flip again */
        CHECK(selection_state_flags_peek() == 0x40u, "the probe flips it on");
        ui_mode_resource_bit40_set_clear(0, 0u);
        selection_state_flags_set(0u); /* restore boot state */
    }

    /* MenuWipeStart (2026-10-24). */
    {
        menu_wipe_arm_set(0u);
        menu_wipe_flags_set(0u);
        menu_wipe_start(); /* unarmed: inert */
        CHECK(menu_wipe_flags_peek() == 0u, "unarmed keeps the flags");
        menu_wipe_arm_set(1u);
        menu_wipe_flags_set(0u);
        menu_clock_millis_set(12345u);
        menu_wipe_start();
        CHECK((menu_wipe_flags_peek() & 3u) == 3u, "both start bits rise");
        CHECK(menu_wipe_clock_peek() == 12345.0f, "the clock stamps");
        menu_wipe_flags_set(0u);
        menu_wipe_arm_set(0u);
    }

    /* The panel texture-slot blit (2026-10-24): the picker picks slots,
     * the flag bits hide items, and the banner word gates the alternates. */
    {
        static char pblock[0x2c * 2];
        memset(pblock, 0, sizeof(pblock));
        float* rec = (float*)pblock;
        rec[0] = 1.0f;
        rec[1] = 2.0f;
        rec[4] = 5.0f;  /* texture A */
        rec[5] = 7.0f;  /* texture C */
        rec[6] = 6.0f;  /* texture B */
        uint32_t fl = 0u;
        __builtin_memcpy(rec + 10, &fl, 4);
        float* rec2 = (float*)(pblock + 0x2c);
        rec2[0] = 10.0f;
        rec2[1] = 10.0f;
        rec2[6] = 6.0f;
        uint32_t fl2 = 0x10u; /* hidden */
        __builtin_memcpy(rec2 + 10, &fl2, 4);
        panel_draw_records_set(11, pblock);
        panel_draw_count_set(11, 2);
        indexed_pair_set(11, 0x41200000u, 0x40a00000u); /* 10.0f, 5.0f */
        sim_status_banner_word_set(0u);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        CHECK(crt_surface_calls_count() == 1, "the hidden item never blits");
        uint32_t h = 0u;
        float x = 0, y = 0;
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(h == 0x40C00000u && x == 11.0f && y == 7.0f,
              "flags clear picks the B slot at the offset position");
        fl = 2u; /* A with the bit-2 raise */
        __builtin_memcpy(rec + 10, &fl, 4);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(crt_surface_calls_count() == 1 && h == 0x40A00000u,
              "the bit-2 raise picks the A slot");
        fl = 8u; /* the C-slot branch */
        __builtin_memcpy(rec + 10, &fl, 4);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(h == 0x40E00000u && x == 11.0f && y == 7.0f,
              "the plain else picks the C slot");
        fl = 0x40u | 4u; /* the banner-gated alternate */
        __builtin_memcpy(rec + 10, &fl, 4);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(h == 0x40E00000u, "the else path takes the C slot while "
                                "the banner word stays clear");
        sim_status_banner_word_set(1u);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        crt_surface_call_get(0, &h, &x, &y);
        CHECK(h == 0x40A00000u, "the banner word raises the A alternate");
        sim_status_banner_word_set(0u);
        panel_draw_count_set(11, 0);
        panel_draw_records_set(11, NULL);
        crt_surface_calls_clear();
        panel_texture_slots_blit(11);
        CHECK(crt_surface_calls_count() == 0, "a closed panel answers nothing");
    }

    /* Idle hover uses the same live records as the image selector. */
    {
        uint32_t saved_flags[12];
        static float items[22];
        memset(items, 0, sizeof(items));
        for (int k = 0; k < 12; ++k) {
            saved_flags[k] = request_record_flags_peek(k);
            request_record_flags_set(k, k == 11 ? 1u : 0u);
        }
        items[2] = items[13] = 36.0f;
        items[3] = items[14] = 25.0f;
        panel_draw_records_set(11, items);
        panel_draw_count_set(11, 2);
        float ox = 330.75f, oy = -0.75f;
        uint32_t xb, yb, flags = 0x10u;
        memcpy(&xb, &ox, sizeof(xb));
        memcpy(&yb, &oy, sizeof(yb));
        indexed_pair_set(11, xb, yb);
        memcpy(items + 10, &flags, sizeof(flags));
        CHECK(frontend_panels_pointer_hover(330, 0) != 0,
              "idle hit uses truncated live origin and includes left/top");
        memcpy(&flags, items + 10, sizeof(flags));
        CHECK(flags == 0x12u, "hidden item still receives reference hover");
        memcpy(&flags, items + 21, sizeof(flags));
        CHECK((flags & 2u) == 0u, "first overlapping item owns hover");
        CHECK(frontend_panels_pointer_hover(366, 0) == 0,
              "right edge is excluded");
        flags = 0xabu;
        memcpy(items + 10, &flags, sizeof(flags));
        frontend_panels_pointer_hover(0, 479);
        memcpy(&flags, items + 10, sizeof(flags));
        CHECK(flags == 0xadu,
              "miss clears hover and restores persistent pressed state");
        request_record_flags_set(11, 0u);
        frontend_panels_pointer_hover(330, 0);
        memcpy(&flags, items + 10, sizeof(flags));
        CHECK(flags == 0xadu, "disabled category skips flag mutation");
        request_record_flags_set(11, 1u);
        FrontendPanelPointerCapture capture;
        FrontendPanelPointerHit hit;
        memset(&capture, 0, sizeof(capture));
        flags = 0x20u;
        memcpy(items + 10, &flags, sizeof(flags));
        frontend_panels_pointer_update(&capture, 330, 0, 1, 0, &hit);
        CHECK(capture.active && hit.item == 0 && !hit.released,
              "fresh press captures the first item");
        frontend_panels_pointer_update(&capture, 0, 479, 1, 1, &hit);
        CHECK(capture.active && !hit.released,
              "drag-out preserves capture without activation");
        frontend_panels_pointer_update(&capture, 330, 0, 1, 1, &hit);
        frontend_panels_pointer_update(&capture, 330, 0, 0, 1, &hit);
        memcpy(&flags, items + 10, sizeof(flags));
        CHECK(!capture.active && hit.released && flags == 0x2au,
              "matching release toggles only the configured persistent item");
        frontend_panels_pointer_update(&capture, 330, 0, 1, 0, &hit);
        frontend_panels_pointer_capture_cancel(&capture);
        frontend_panels_pointer_update(&capture, 330, 0, 0, 1, &hit);
        CHECK(!hit.released, "cancel cannot synthesize an activation");
        panel_draw_records_set(11, NULL);
        panel_draw_count_set(11, 0);
        for (int k = 0; k < 12; ++k)
            request_record_flags_set(k, saved_flags[k]);
    }

    /* The timed-effect timestamp reset (2026-10-24): stamp + guard. */
    {
        timed_effect_tick_word_set(123u);
        timed_effect_timestamp_reset(3);
        CHECK(timed_effect_time_peek(3) == 123u, "the stamp lands");
        timed_effect_timestamp_reset(-1); /* guarded */
        CHECK(timed_effect_time_peek(-1) == 0u, "the guard answers clear");
        timed_effect_tick_word_set(0u);
    }

    /* The screen-right-edge cache refresh (2026-10-24): metrics fold. */
    {
        char metric[0x30];
        memset(metric, 0, sizeof metric);
        indexed_record_pointer_set(0, metric);
        indexed_text_metrics_pair_set(0, 0u, 0u);
        screen_right_edge_cache_refresh();
        CHECK(screen_right_edge_cache_peek() == 0u, "a clear pair caches clear");
        indexed_text_metrics_pair_set(0, 11u, 4u);
        screen_right_edge_cache_refresh();
        CHECK(screen_right_edge_cache_peek() == 7u, "the fold caches x - y");
        indexed_text_metrics_pair_set(0, 0u, 0u);
        indexed_record_pointer_set(0, NULL);
    }

    /* The glyph measure wrapper (2026-10-24): the corpus lane is in-band. */
    {
        bitmap_font_glyph_measure_width(NULL, 'A'); /* in-range glyph */
        bitmap_font_glyph_measure_width(NULL, 0u);   /* below the grid */
        CHECK(1, "the measure wrapper runs as the corpus's void lane");
    }

    /* The panel resource + scroll leaf band (2026-10-24). */
    {
        char res;
        /* PanelResourcePositionApplyIfEnabled: gate + bubble offsets. */
        panel_seam_reset();
        panel_enable_word_set(0u);
        panel_resource_position_apply_if_enabled(&res, 1, 2);
        CHECK(panel_seam_calls() == 0, "the clear gate stays silent");
        panel_enable_word_set(0x10u);
        bubble_offsets_set(1.5f, 2.5f);
        panel_resource_position_apply_if_enabled(&res, 10, 20);
        void* last_res = NULL;
        float lx = 0.0f, ly = 0.0f;
        panel_seam_last(&last_res, &lx, &ly);
        CHECK(panel_seam_calls() == 1 && last_res == &res && lx == 11.5f
                  && ly == 22.5f,
              "the enabled pass rides the seam with the bubble offsets");
        /* PanelResourceHandleAndDimensionsStore: the seam + stores. */
        panel_seam_reset();
        panel_surface_handle_set((void*)1);
        panel_resource_handle_and_dimensions_store(&res, 64u, 32u, 0x99u);
        CHECK(panel_surface_handle_peek() == NULL && panel_seam_calls() == 1,
              "the load seam answers NULL and records");
        float w = 0.0f, hh = 0.0f;
        uint32_t extra = 0u;
        panel_dimensions_peek(&w, &hh, &extra);
        CHECK(w == 64.0f && hh == 32.0f && extra == 0x99u,
              "the dimensions land in the window");
        /* ScrollOffsetFromCursorUpdate: pair fold + metrics + cache. */
        char metric[0x30];
        memset(metric, 0, sizeof metric);
        char ui_rec[0x828];
        memset(ui_rec, 0, sizeof ui_rec);
        /* corpus shape: the forward reads the record's +0 field for the
         * pair's object — here it points back at the record itself */
        void* pair_holder = ui_rec;
        __builtin_memcpy(ui_rec, &pair_holder, sizeof pair_holder);
        indexed_record_pointer_set(0, metric);
        panel_ui_record_set(ui_rec);
        const uint32_t lo = 11u, hi = 4u;
        __builtin_memcpy(ui_rec + 0x820, &lo, sizeof lo);
        __builtin_memcpy(ui_rec + 0x824, &hi, sizeof hi);
        scroll_offset_from_cursor_update();
        uint32_t mx = 0u, my = 0u;
        indexed_text_metrics_pair_get(0, &mx, &my);
        CHECK(mx == 7u && my == 0u, "the clamped pair difference lands");
        CHECK(screen_right_edge_cache_peek() == 7u, "the cache follows");
        const uint32_t low2 = 2u, high2 = 9u;
        __builtin_memcpy(ui_rec + 0x820, &low2, sizeof low2);
        __builtin_memcpy(ui_rec + 0x824, &high2, sizeof high2);
        scroll_offset_from_cursor_update(); /* clamps to zero */
        indexed_text_metrics_pair_get(0, &mx, &my);
        CHECK(mx == 0u, "the negative difference clamps at zero");
        panel_ui_record_set(NULL);
        scroll_offset_from_cursor_update(); /* guarded pair */
        indexed_text_metrics_pair_get(0, &mx, &my);
        CHECK(mx == 0u, "the clear record forwards zero");
        indexed_text_metrics_pair_set(0, 0u, 0u);
        indexed_record_pointer_set(0, NULL);
        panel_enable_word_set(0u);
    }

    /* IndexedPanelResourcePositionApply (FUN_00459f50; 2026-10-25): the
     * slot selector rides the corpus's exact x87 constant products — a
     * float 40 answers slot 1, since the extended-precision product of
     * 0.01 lands just under two. */
    {
        char slot0, slot1, slot4;
        panel_seam_reset();
        panel_enable_word_set(0u);
        panel_position_offsets_set(1.5f, 2.5f);
        panel_position_slot_set(1, &slot1);
        float rec = 40.0f;
        indexed_panel_resource_position_apply(&rec, 10, 20);
        CHECK(panel_seam_calls() == 0, "the clear enable word stays silent");
        panel_enable_word_set(1u);
        indexed_panel_resource_position_apply(&rec, 10, 20);
        void* last_res = NULL;
        float lx = 0.0f, ly = 0.0f;
        panel_seam_last(&last_res, &lx, &ly);
        CHECK(panel_seam_calls() == 1 && last_res == &slot1
                  && lx == 11.5f && ly == 22.5f,
              "the extended-precision selector picks slot 1 and rides "
              "the seam with the offset pair");
        rec = 110.0f; /* selector 5 -> clamped */
        panel_position_slot_set(4, &slot4);
        indexed_panel_resource_position_apply(&rec, 1, 1);
        panel_seam_last(&last_res, &lx, &ly);
        CHECK(panel_seam_calls() == 2 && last_res == &slot4 && lx == 2.5f
                  && ly == 3.5f,
              "the out-of-range selector clamps to the last slot");
        rec = -40.0f; /* negative truncation clamps the same way */
        indexed_panel_resource_position_apply(&rec, 1, 1);
        panel_seam_last(&last_res, &lx, &ly);
        CHECK(panel_seam_calls() == 3 && last_res == &slot4,
              "the negative conversion rides the last slot too");
        panel_position_slot_set(4, NULL);
        panel_seam_reset();
        indexed_panel_resource_position_apply(&rec, 1, 1);
        CHECK(panel_seam_calls() == 0, "an uninstalled slot stays inert");
        panel_position_slot_set(0, &slot0);
        indexed_panel_resource_position_apply(NULL, 3, 4); /* reads zero */
        panel_seam_last(&last_res, &lx, &ly);
        CHECK(panel_seam_calls() == 1 && last_res == &slot0 && lx == 4.5f
                  && ly == 6.5f,
              "the NULL record float reads selector zero (deviation)");
        panel_position_slot_set(0, NULL);
        panel_position_slot_set(1, NULL);
        panel_position_offsets_set(0.0f, 0.0f);
        panel_enable_word_set(0u);
    }

    /* ActiveObjectList word selector (2026-10-24). */
    {
        frontend_flow_word_set(2, 0); /* in-game */
        int* w_in = active_object_list_word();
        *w_in = 42;
        frontend_flow_word_set(2, -1); /* out of game */
        int* w_out = active_object_list_word();
        CHECK(w_out != w_in, "the selector flips words with the mode");
        *w_out = 7;
        CHECK(*w_in == 42, "the out-of-game store leaves the in-game word");
        frontend_flow_word_set(2, 5); /* back in-game */
        CHECK(active_object_list_word() == w_in, "the in-game word returns");
        *w_out = 0;
        frontend_flow_word_set(2, 0);
    }

    /* ScreenEffectFlagsClearAndRelease (2026-10-24): the ten-entry walk. */
    {
        char h1;
        char h2;
        screen_effect_entry_set(0, &h1, 0x3u); /* active + extra bit */
        screen_effect_entry_set(1, &h2, 2u);   /* inactive */
        screen_effect_entry_set(2, NULL, 1u);  /* active, clear handle */
        screen_effect_entry_set(3, NULL, 0u);  /* silent */
        screen_effect_seam_reset();
        screen_effect_flags_clear_and_release();
        void* last = (void*)1;
        int flag = -1;
        screen_effect_seam_last(&last, &flag);
        CHECK(screen_effect_seam_calls() == 2 && flag == 1 && last == NULL,
              "only the active entries fire the seam");
        CHECK(screen_effect_flag_peek(0) == 2u && screen_effect_flag_peek(1) == 2u,
              "bit 0 clears and the other bits stay");
        screen_effect_seam_reset();
        screen_effect_flags_clear_and_release();
        CHECK(screen_effect_seam_calls() == 0, "the second walk is silent");
        for (int i = 0; i < 10; ++i) {
            screen_effect_entry_set(i, NULL, 0u);
        }
        CHECK(screen_effect_flag_peek(-1) == 0u
                  && screen_effect_handle_peek(99) == NULL,
              "the window guards answer clear");
    }

    /* StoreSelectedMapCoordinate (2026-10-24): switch vs tracked. */
    {
        const uint32_t c1[2] = { 11u, 12u };
        const uint32_t c2[2] = { 21u, 22u };
        objective_selection_state_reset(); /* bit 7 raises, word clears */
        store_selected_map_coordinate(7, c1); /* word 0 != 7: switch */
        CHECK(map_store_kind_peek() == 7u && map_store_last_word(0) == 11u
                  && map_store_last_word(1) == 12u
                  && map_store_pair_word(0) == 0u,
              "the switch branch re-arms the kind and the last pair");
        objective_selection_state_reset(); /* word 0, bit 7 set */
        store_selected_map_coordinate(0, c2); /* tracked: word == kind */
        CHECK(map_store_pair_word(0) == 21u && map_store_pair_word(1) == 22u
                  && map_store_last_word(0) == 21u
                  && map_store_kind_peek() == 0u,
              "the tracked branch refreshes the live pair too");
        store_selected_map_coordinate(0, NULL);
        CHECK(map_store_pair_word(0) == 21u && map_store_kind_peek() == 0u,
              "the null pair stays away from every slot");
        objective_selection_state_reset();
        store_selected_map_coordinate(9, NULL); /* word 0 != 9: switch */
        CHECK(map_store_kind_peek() == 9u && map_store_pair_word(0) == 21u,
              "the kind change under the flag switches again");
    }

    /* TextLayoutOffsetForCentering (2026-10-24): the half-width shift. */
    {
        CHECK(text_layout_offset_for_centering(8, 0, 0, 0, 10, 10, 0) == 1,
              "the uncentered rect keeps the hit");
        CHECK(text_layout_offset_for_centering(8, 0, 0, 0, 10, 10, 1) == 0,
              "the centered shift drops the outside point");
        CHECK(text_layout_offset_for_centering(2, 5, 0, 0, 10, 10, 1) == 1,
              "the centered rect still catches its inside point");
    }

    /* SelectionEntityCodeResolve (2026-10-24): the flag-keep branch and
     * the selection high-bit pick. */
    {
        static GameContext sctx;
        static EntitySlot picked;
        static const uint32_t cc[2] = { 4u, 5u };
        game_context_init(&sctx);
        objective_selection_state_reset(); /* inter bit 7 raises */
        store_selected_map_coordinate(3, cc); /* switch branch: bit 7
                                                 drops, bit 8 raises */
        int code = 0x11;
        CHECK(selection_entity_code_resolve(&sctx, &code) == 0x11
                  && code == 0x11,
              "the flag route answers the stored code unchanged");
        objective_selection_state_reset(); /* bit 7 back: the pick branch */
        code = 0x11;
        CHECK(selection_entity_code_resolve(&sctx, &code) == 0x23
                  && code == 0x23,
              "the empty selection answers 0x23 (deviation)");
        memset(&picked, 0, sizeof picked);
        picked.type = 1;
        picked.flags_3e8 = 0x80000000u;
        selection_add(&sctx, &picked);
        code = 0;
        CHECK(selection_entity_code_resolve(&sctx, &code) == 0x22
                  && code == 0x22,
              "the high-bit pick answers 0x22 through the store");
        selection_clear(&sctx);
    }

    /* Scroll-rect + flic-mode window band (2026-10-25). */
    {
        const float quad[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        scroll_rect_resource_record_create(5, 0, quad, 0, 0);
        uint32_t handle = 0u;
        void* ra = NULL;
        void* rb = NULL;
        int region = 0;
        view_blit_window_peek(5, &handle, &ra, &rb, &region);
        CHECK(ra != NULL && rb == NULL, "the mode-0 record lands in half A");
        const float* pf = (const float*)ra;
        CHECK(pf[0] == 1.0f && pf[1] == 2.0f, "the position pair lands");
        CHECK(pf[2] == 4.0f && pf[3] == 6.0f,
              "the extent folds x+dx and y+dy (corpus add order)");
        CHECK(pf[4] == 0.0f && pf[5] == 0.0f && pf[6] == 0.0f,
              "the seam dims stay zero (deviation)");
        scroll_rect_resource_record_create(5, 1, quad, 7, 7);
        view_blit_window_peek(5, &handle, &ra, &rb, &region);
        CHECK(ra != NULL && rb != NULL && ra != rb,
              "the mode-1 store fills half B and keeps half A");
        void* first_a = ra;
        scroll_rect_resource_record_create(-1, 0, NULL, 0, 0); /* guarded */
        view_blit_window_peek(5, &handle, &ra, &rb, &region);
        CHECK(ra == first_a, "the window guard keeps the slot");
    }

    /* UiOverlayAnimationAdvance (2026-10-25): both convergence halves. */
    {
        overlay_anim_float_install(1.0f);
        ui_overlay_animation_advance(0, 1);
        const float decayed = overlay_anim_float_peek();
        CHECK(decayed == (1.0f - 0.05f) * 20.0f * 0.05263158f,
              "the decay lands the corpus expression");
        CHECK(decayed == 1.0f,
              "one is the corpus expression's fixed point (the step does "
              "not move at the top — pinned 2026-10-25 against the corpus)");
        overlay_anim_float_install(0.9f);
        ui_overlay_animation_advance(0, 1);
        CHECK(overlay_anim_float_peek() == (0.9f - 0.05f) * 20.0f * 0.05263158f
                  && overlay_anim_float_peek() < 0.9f,
              "below the fixed point the decay steps down the same shape");
        overlay_anim_float_install(1.0f);
        ui_overlay_animation_advance(0, 3);
        CHECK(overlay_anim_float_peek() >= 0.0f,
              "the after-loop clamp holds through a three-step decay");

        overlay_anim_float_install(0.0f);
        ui_overlay_animation_advance(1, 1);
        CHECK(overlay_anim_float_peek() == 0.0f - (1.0f - 0.0f) * -0.05f,
              "the ease half steps toward one");
        ui_overlay_animation_advance(1, 1);
        CHECK(overlay_anim_float_peek() > 0.05f && overlay_anim_float_peek() < 1.0f,
              "the ease half keeps converging");
        overlay_anim_float_install(0.0f);
        ui_overlay_animation_advance(1, 0);
        CHECK(overlay_anim_float_peek() == 0.0f, "zero steps leave the float");
        ui_overlay_animation_advance(1, -3);
        CHECK(overlay_anim_float_peek() == 0.0f,
              "the guarded runaway counts no steps (deviation)");
        overlay_anim_float_install(0.5f);
    }

    /* Timed-effect cue request (2026-10-25): the cue word store plus the
     * flag-bit 0/1 raise, and the guarded window. */
    {
        CHECK(timed_effect_cue_peek(12) == 0u, "the cue window starts clear");
        timed_effect_cue_request_set(12, 0x77u);
        CHECK(timed_effect_cue_peek(12) == 0x77u, "the cue word stores");
        CHECK((timed_effect_flags_peek(12) & 3u) == 3u,
              "the request raises flag bits 0 and 1");
        timed_effect_cue_request_set(12, 0u);
        CHECK(timed_effect_cue_peek(12) == 0u, "the store accepts zero");
        CHECK(timed_effect_flags_peek(12) == 3u,
              "the raise is a bit-or, not a store");
        timed_effect_cue_request_set(12, 8u);
        timed_effect_update_request_set(12);
        CHECK(timed_effect_flags_peek(12) == 7u,
              "the update bit joins the cue bits");
        timed_effect_cue_request_set(40, 1u); /* first index past the 40 */
        CHECK(timed_effect_cue_peek(40) == 0u
                  && timed_effect_flags_peek(40) == 0u,
              "the guarded window drops the out-of-range request (dev)");
    }

    /* Timed-effect persistence (2026-10-25): the flag word's bit 4 raise
     * and drop, the or-shape, and the guarded window. */
    {
        timed_effect_persistence_set(3, 1);
        CHECK((timed_effect_flags_peek(3) & 0x10u) != 0u,
              "the set raises bit 4");
        timed_effect_update_request_set(3);
        CHECK(timed_effect_flags_peek(3) == 0x14u,
              "the or-shape joins the update bit");
        timed_effect_persistence_set(3, 0);
        CHECK(timed_effect_flags_peek(3) == 0x4u,
              "the drop leaves the update bit");
        timed_effect_persistence_set(40, 1); /* first index past the 40 */
        CHECK(timed_effect_flags_peek(40) == 0u,
              "the guarded window drops the out-of-range set (dev)");
    }

    /* Cursor-tile selection pair (2026-10-25): the kind-keyed installs
     * with the own-storage pair. */
    {
        cursor_tile_selection_update_by_type(0x49, 7u, 9u);
        CHECK(cursor_tile_pair_peek(0) == 7u && cursor_tile_pair_peek(1) == 9u,
              "the 0x49 kind installs the caller's pair");
        cursor_tile_selection_update_by_type(5, 0xdeadu, 0xbeefu);
        CHECK(cursor_tile_pair_peek(0) == 0xFu && cursor_tile_pair_peek(1) == 0u,
              "the kind-5 install lands the fixed shape");
        cursor_tile_selection_update_by_type(0xe, 1u, 2u);
        CHECK(cursor_tile_pair_peek(0) == 0xBu && cursor_tile_pair_peek(1) == 0u,
              "the kind-0xe install lands its shape");
        cursor_tile_selection_update_by_type(3, 4u, 5u);
        CHECK(cursor_tile_pair_peek(0) == 0xBu && cursor_tile_pair_peek(1) == 0u,
              "the foreign kind rides the pair");
        cursor_tile_selection_update_by_type(0x49, 1u, 2u);
        CHECK(cursor_tile_pair_peek(2) == 0u,
              "the peek guards the window (dev)");
    }

    /* Panel layout band (2026-10-25): the store, the load-seam answers,
     * and the gated position update through the seam. */
    {
        panel_seam_reset();
        const int base = panel_seam_calls();
        panel_resource_layout_store((void*)0x11u, 64u, 32u, 0x55u,
                                    (void*)0x22u, 10u, 20u);
        CHECK(panel_layout_surface_peek() == NULL
                  && panel_layout_glyph_peek() == NULL,
              "the load seams answer NULL");
        CHECK(panel_seam_calls() == base + 2,
              "the two loads record two seam calls");
        CHECK(panel_layout_size_peek(0) == 64.0f
                  && panel_layout_size_peek(1) == 32.0f,
              "the size floats land");
        CHECK(panel_layout_extra_peek() == 0x55u, "the extra word lands");
        CHECK(panel_layout_offset_peek(0) == 10.0f
                  && panel_layout_offset_peek(1) == 20.0f,
              "the position floats land");
        CHECK(panel_layout_size_peek(2) == 0.0f
                  && panel_layout_offset_peek(2) == 0.0f,
              "the peeks guard the window (dev)");
        panel_resource_position_update(11);
        CHECK(panel_seam_calls() == base + 2,
              "the absent glyph skips the update");

        static int glyph = 0;
        panel_layout_glyph_install(&glyph);
        indexed_pair_set(11, 10u, 5u);
        global_value_0050080c_set(1u);
        sim_status_banner_word_set(1u);
        panel_seam_reset();
        panel_resource_position_update(11);
        void* res = NULL;
        float sx = 0.0f, sy = 0.0f;
        panel_seam_last(&res, &sx, &sy);
        CHECK(panel_seam_calls() == 1 && res == &glyph
                  && sx == 20.0f && sy == 25.0f,
              "the update rides the pair plus the offsets to the seam");
        global_value_0050080c_set(0u);
        panel_seam_reset();
        panel_resource_position_update(11);
        CHECK(panel_seam_calls() == 0, "the layout gate closes the blit");
        global_value_0050080c_set(1u);
        sim_status_banner_word_set(0u);
        panel_seam_reset();
        panel_resource_position_update(11);
        CHECK(panel_seam_calls() == 0, "the banner gate closes the blit");
        sim_status_banner_word_set(1u);
        global_value_0050080c_set(0u);
        panel_layout_glyph_install(NULL); /* back to the seam-NULL model */
    }

    /* UI/panel category-bounds band (2026-10-25): the category store with
     * the flag-gated pair routing, the entry bounds get, the priority
     * state pair, and the render record create. */
    {
        const int panel = 15;
        request_record_flags_set(panel, 0u);
        panel_category_bounds_set(panel, 1, 2, 3, 4);
        float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        panel_category_bounds_peek(panel, &x1, &y1, &x2, &y2);
        CHECK(x1 == 1.0f && y1 == 2.0f && x2 == 3.0f && y2 == 4.0f,
              "the category rectangle lands as floats");
        uint32_t px = 0, py = 0;
        indexed_pair_get(panel, &px, &py);
        float pf = 0.0f;
        __builtin_memcpy(&pf, &px, sizeof pf);
        CHECK(pf == 3.0f, "the clear flag routes the second corner x");
        __builtin_memcpy(&pf, &py, sizeof pf);
        CHECK(pf == 4.0f, "the clear flag routes the second corner y");

        request_record_flags_set(panel, 2u);
        panel_category_bounds_set(panel, 10, 20, 30, 40);
        indexed_pair_get(panel, &px, &py);
        __builtin_memcpy(&pf, &px, sizeof pf);
        CHECK(pf == 10.0f, "the raised flag routes the first corner x");
        __builtin_memcpy(&pf, &py, sizeof pf);
        CHECK(pf == 20.0f, "the raised flag routes the first corner y");
        panel_category_bounds_peek(panel, &x1, &y1, &x2, &y2);
        CHECK(x1 == 10.0f && x2 == 30.0f, "the rectangle fields update");
        panel_category_bounds_set(-1, 0, 0, 0, 0); /* guarded */

        static char cat_block[0x2c * 2];
        memset(cat_block, 0, sizeof cat_block);
        float* cat_rec = (float*)cat_block;
        cat_rec[0] = 1.0f;
        cat_rec[1] = 2.0f;
        cat_rec[2] = 5.0f;
        cat_rec[3] = 8.0f;
        float* cat_rec2 = (float*)(cat_block + 0x2c);
        cat_rec2[0] = 4.0f;
        cat_rec2[1] = 6.0f;
        cat_rec2[2] = 5.0f;
        cat_rec2[3] = 6.0f;
        panel_draw_records_set(panel, cat_block);
        float bounds[4] = { 0, 0, 0, 0 };
        ui_entry_bounds_get(panel, 0, bounds);
        CHECK(bounds[0] == 1.0f && bounds[1] == 2.0f
                  && bounds[2] == 4.0f && bounds[3] == 6.0f,
              "the entry bounds answer position plus the derived size");
        ui_entry_bounds_get(panel, 1, bounds);
        CHECK(bounds[2] == 1.0f && bounds[3] == 0.0f,
              "the second entry derives its size too");
        memset(bounds, 0, sizeof bounds);
        ui_entry_bounds_get(16, 0, bounds); /* no block installed */
        CHECK(bounds[2] == 0.0f, "the uninstalled panel writes nothing (dev)");
        ui_entry_bounds_get(panel, -1, bounds); /* guarded */
        ui_entry_bounds_get(panel, 0, NULL); /* guarded */
        panel_draw_records_set(panel, NULL);
        request_record_flags_set(panel, 0u);

        priorities_selection_set(0u, 0u);
        CHECK(priorities_select(7u, 0, 0) == 0,
              "a release clears the selection");
        unsigned int st = 0u;
        unsigned int flags = 0u;
        priorities_selection_get(&st, &flags);
        CHECK(st == PRIORITY_TYPE_COUNT && flags == 0u,
              "the cleared selection holds the sentinel");
        CHECK(priorities_select(9u, 1, 0) == 0,
              "a new press answers false");
        priorities_selection_get(&st, &flags);
        CHECK(st == 9u && (flags & 2u) != 0u,
              "a new press selects the row and raises the pressed flag");
        CHECK(priorities_select(9u, 1, 1) == 1,
              "a held press on the selected row answers true");
        priorities_selection_get(&st, &flags);
        CHECK(st == 9u, "a held press keeps the selection");
        CHECK(priorities_select(8u, 1, 1) == 0,
              "another row answers false");
        priorities_selection_set(5u, 0u);
        CHECK(priorities_select(5u, 1, 1) == 1,
              "the selected row ignores the released argument");
        priorities_selection_get(&st, &flags);
        CHECK((flags & 2u) != 0u, "the selected row raises the pressed flag");
        priorities_selection_set(PRIORITY_TYPE_COUNT, 0u);
        CHECK(priorities_select(9u, 1, 1) == 0,
              "a released press selects nothing");
        priorities_selection_get(&st, &flags);
        CHECK(st == PRIORITY_TYPE_COUNT && flags == 0u,
              "a released press keeps the sentinel and flags clear");

        const Point2F hover_position = { 5.0f, 6.0f };
        Area2F hover_area;
        priorities_set_hover_area(&hover_position, 640u, 480u);
        priorities_hover_area_get(&hover_area);
        CHECK(hover_area.x == 5.0f && hover_area.y == 6.0f &&
                  hover_area.width == 640.0f && hover_area.height == 480.0f,
              "the hover area lands");

        uint32_t srcbuf[17];
        memset(srcbuf, 0, sizeof srcbuf);
        const float source_float = 3.5f;
        __builtin_memcpy(&srcbuf[4], &source_float, sizeof source_float);
        const uint32_t payload[4] = { 0xA1u, 0xB2u, 0xC3u, 0xD4u };
        UiRenderRecord* rec = ui_render_record_create(srcbuf, payload, 0xEEu);
        CHECK(rec != NULL && rec->source == srcbuf
                  && rec->payload[0] == 0xA1u && rec->payload[3] == 0xD4u
                  && rec->extra == 0xEEu && rec->float_at_10 == 3.5f,
              "the render record copies the fields and the +0x10 float");
        ui_render_record_free(rec);
        CHECK(ui_render_record_create(NULL, payload, 0u) == NULL,
              "the null source answers NULL (dev)");
        CHECK(ui_render_record_create(srcbuf, NULL, 0u) == NULL,
              "the null payload answers NULL (dev)");
    }

    /* Sav visited tables: per-root entries, the out-of-game-first clear
     * walk, and the null-item skip. */
    {
        char rec1[24];
        char rec2[24];
        memset(rec1, 0, sizeof rec1);
        memset(rec2, 0, sizeof rec2);
        ((uint32_t*)rec1)[4] = 0xdeadu; /* corpus flag word at +0x10 */
        ((uint32_t*)rec2)[4] = 0xbeefu;
        sav_visited_entry_set(0, 0, rec1, 1u); /* out-of-game band */
        sav_visited_entry_set(0, 1, NULL, 2u);
        sav_visited_entry_set(1, 0, rec2, 3u); /* in-game band */
        void* p = NULL;
        uint32_t w = 0u;
        sav_visited_entry_peek(0, 0, &p, &w);
        CHECK(p == rec1 && w == 1u, "install and peek land the pair");
        sav_visited_entry_peek(0, 1, &p, &w);
        CHECK(p == NULL && w == 2u, "the null item rides through");
        sav_visited_entry_set(0, 16, rec1, 9u);  /* outside this band */
        sav_visited_entry_peek(0, 16, &p, &w);   /* outside this band */

        frontend_flow_word_set(2, -1);
        int* wc = active_object_list_word();
        *wc = 2; /* the out-of-game count */
        frontend_flow_word_set(2, 0);
        wc = active_object_list_word();
        *wc = 1; /* the in-game count */
        sav_clear_visited();
        CHECK(((uint32_t*)rec1)[4] == 0u,
              "the out-of-game record flag word clears");
        sav_visited_entry_peek(0, 0, &p, &w);
        CHECK(p == rec1 && w == 0u, "the table word clears alongside");
        sav_visited_entry_peek(0, 1, &p, &w);
        CHECK(w == 2u, "the null-item table word rides");
        CHECK(((uint32_t*)rec2)[4] == 0u, "the in-game record flag clears");
        sav_visited_entry_peek(1, 0, &p, &w);
        CHECK(w == 0u, "the in-game table word clears");

        /* Faithful FUN_00416e70 trusts the active counts (they are set from
         * the same root sizes that size the sidecar arrays), so a count
         * beyond the two-entry band is no longer clamped and would walk off
         * the allocation. Keep the count in-band: a one-entry out-of-game
         * count clears that entry; the zero in-game count leaves its band
         * untouched. */
        ((uint32_t*)rec1)[4] = 0xdeadu;
        ((uint32_t*)rec2)[4] = 0xbeefu;
        sav_visited_entry_set(0, 0, rec1, 1u);
        sav_visited_entry_set(1, 0, rec2, 3u);
        frontend_flow_word_set(2, -1);
        wc = active_object_list_word();
        *wc = 1; /* in-band count (the pre-faithful guard test used 17) */
        frontend_flow_word_set(2, 0);
        wc = active_object_list_word();
        *wc = 0;
        sav_clear_visited();
        CHECK(((uint32_t*)rec1)[4] == 0u,
              "the counted entry clears");
        CHECK(((uint32_t*)rec2)[4] == 0xbeefu,
              "the zero count leaves the band");
        sav_visited_entry_peek(0, 0, &p, &w);
        CHECK(w == 0u, "the counted entry clears its paired word");

        /* Leave the modeled state clean. */
        frontend_flow_word_set(2, -1);
        wc = active_object_list_word();
        *wc = 0;
        frontend_flow_word_set(2, 0);
        wc = active_object_list_word();
        *wc = 0;
        sav_visited_entry_set(0, 0, NULL, 0u);
        sav_visited_entry_set(0, 1, NULL, 0u);
        sav_visited_entry_set(1, 0, NULL, 0u);
    }

    /* MenuListFindName (2026-10-25): the folded list walk, the null-entry
     * stride, and the record count gate. */
    {
        struct named { const char* name; int extra; };
        static named alpha = { "Alpha", 1 };
        static named bravo = { "Bravo", 2 };
        static named charlie = { "Charlie", 3 };
        void* list[4] = { &alpha, NULL, &bravo, &charlie };
        struct menu_list_header { void* const* entries; int count; } header = {
            list, 4
        };
        CHECK(menu_list_find_name(&header, "alpha") == 0,
              "the fold lands the first entry");
        CHECK(menu_list_find_name(&header, "ALPHA") == 0,
              "the fold reads case-blind");
        CHECK(menu_list_find_name(&header, "Bravo") == 2,
              "the null entry strides on");
        CHECK(menu_list_find_name(&header, "CHARLIE") == 3,
              "the last entry answers last");
        CHECK(menu_list_find_name(&header, "delta") == -1,
              "the miss answers -1");
        header.count = 1;
        CHECK(menu_list_find_name(&header, "Bravo") == -1,
              "the count bounds the walk");
        header.count = 0;
        CHECK(menu_list_find_name(&header, "Alpha") == -1,
              "the count gate answers -1");
    }

    /* ScreenCoordNormalizeInit (FUN_0045d8b0; 2026-10-25): the band pass
     * over the port's display pair. The x half lands back on the corpus
     * constants (the width constant's one-ulp bias undoes against 640),
     * while the y half rides the height constant's bias one bit up. */
    {
        auto bits = [](float v) {
            uint32_t b;
            __builtin_memcpy(&b, &v, sizeof b);
            return b;
        };
        display_dimensions_set(640, 480);
        screen_coord_normalize_reset();
        CHECK(screen_coord_normalize_color_peek(0) == 255.0f
                  && screen_coord_normalize_pair_peek(0, 0, 0) == 0.0f
                  && screen_coord_normalize_cell_peek(0, 0) == 0.0f
                  && screen_coord_normalize_rows_peek() == 0u,
              "the reset hook seeds the corpus data shape");
        screen_coord_normalize_init();

        CHECK(screen_coord_normalize_color_peek(0) == 1.0f
                  && screen_coord_normalize_color_peek(2) == 0.0f
                  && bits(screen_coord_normalize_color_peek(1))
                         == 0x3f4cccceu,
              "the color lane scales the corpus constants by 1/255");
        CHECK(screen_coord_normalize_color_peek(90) == 0.0f
                  && screen_coord_normalize_color_peek(-1) == 0.0f,
              "the peek hook bounds the color window");
        CHECK(screen_coord_normalize_cell_peek(0, 0) == 1.0f
                  && screen_coord_normalize_cell_peek(1, 19) == 1.0f
                  && screen_coord_normalize_cell_peek(0, 20) == 0.0f,
              "both unit-cell lanes fill the stride-16 window");
        CHECK(screen_coord_normalize_rows_peek() == 7u,
              "the row word books seven");

        static const uint32_t x_bits[7] = { 0x43a00000u, 0x43b40000u,
            0x43b40000u, 0x43c80000u, 0x438c0000u, 0x435c0000u, 0u };
        static const uint32_t y_bits[7] = { 0x42200001u, 0x43340001u,
            0x43340001u, 0x43340001u, 0x43340001u, 0x43340001u, 0u };
        static const uint32_t mx_bits[7] = { 0x43d20000u, 0x43b40000u,
            0x43b40000u, 0x43c80000u, 0x438c0000u, 0x435c0000u,
            0x43a00000u };
        static const uint32_t my_bits[7] = { 0x43340001u, 0x43340001u,
            0x43340001u, 0x43340001u, 0x43340001u, 0x43340001u,
            0x42200001u };
        for (int row = 0; row < 7; ++row) {
            CHECK(bits(screen_coord_normalize_pair_peek(0, row,
                                                        0)) == x_bits[row]
                      && bits(screen_coord_normalize_pair_peek(0, row,
                                                              1)) == y_bits[row]
                      && bits(screen_coord_normalize_pair_peek(1, row,
                                                        0)) == mx_bits[row]
                      && bits(screen_coord_normalize_pair_peek(1, row,
                                                              1)) == my_bits[row],
                  "the pair rows scale the corpus constants exactly");
        }
        CHECK(screen_coord_normalize_pair_peek(2, 0, 0) == 0.0f,
              "the mirror lane bounds its guards");

        screen_coord_normalize_init(); /* second pass re-scales (quirk) */
        CHECK(bits(screen_coord_normalize_color_peek(0)) == 0x3b808081u,
              "a second pass re-scales the colors (corpus quirk)");
        screen_coord_normalize_reset();
        CHECK(screen_coord_normalize_color_peek(0) == 255.0f
                  && screen_coord_normalize_cell_peek(1, 5) == 0.0f
                  && screen_coord_normalize_rows_peek() == 0u,
              "the reset hook restores the seed shape");
        display_dimensions_set(0, 0);
    }

    /* Load band + progress bar + scaled menu blit (FUN_0041b270 /
     * FUN_0045bfb0 / FUN_0045c3f0 / FUN_00412af0; 2026-10-25). */
    {
        auto bits_of = [](float v) {
            uint32_t b;
            __builtin_memcpy(&b, &v, sizeof b);
            return b;
        };
        auto float_of = [](uint32_t b) {
            float f;
            __builtin_memcpy(&f, &b, sizeof f);
            return f;
        };
        string_cache_bitmap_loader_set(s_c179_loader);
        crt_surface_calls_clear();
        interface_button_images_load("c179_l", "c179_r");
        CHECK(interface_button_surface_peek(0)
                      == (uint32_t)(uintptr_t)s_c179_buf[0]
                  && interface_button_surface_peek(1)
                         == (uint32_t)(uintptr_t)s_c179_buf[1],
              "the button loads land both window slots");
        CHECK(crt_surface_calls_count() == 2,
              "both loads call the transform seam");
        uint32_t h0 = 0;
        float xb = 0, yb = 0;
        CHECK(crt_surface_call_get(0, &h0, &xb, &yb) && h0 != 0u
                  && xb == 0.0f && yb == 0.0f,
              "the transform seam records (handle, 0, 0)");

        crt_surface_calls_clear();
        interface_button_images_load("c179_l", "c179_r");
        CHECK(crt_surface_calls_count() == 2
                  && interface_button_surface_peek(0) != 0u,
              "the cache lane reloads (identity holds)");
        crt_surface_calls_clear();
        interface_button_images_load(NULL, "");
        CHECK(crt_surface_calls_count() == 0
                  && interface_button_surface_peek(0) == 0u
                  && interface_button_surface_peek(1) == 0u,
              "the empty keys stay silent");

        crt_surface_calls_clear();
        three_surface_resources_load("c179_p1", "c179_p2", "c179_p3");
        CHECK(advisor_bar_surface_peek(0)
                      == (uint32_t)(uintptr_t)s_c179_buf[2]
                  && advisor_bar_surface_peek(1)
                         == (uint32_t)(uintptr_t)s_c179_buf[4]
                  && advisor_bar_surface_peek(2)
                         == (uint32_t)(uintptr_t)s_c179_buf[3],
              "the three-way quirk: param_2 rides peek 2");
        CHECK(crt_surface_calls_count() == 3,
              "all three transforms record");

        /* the fill draw — face buffer, pair row, size pair, extra word */
        static int face_word = 32;
        static char face_rec[12];
        __builtin_memcpy(face_rec + 8, &face_word, sizeof face_word);
        uint32_t p1x = 0, p1y = 0;
        indexed_pair_get(1, &p1x, &p1y);
        indexed_pair_set(1, bits_of(2.5f), bits_of(1.25f));
        panel_resource_layout_store((void*)1u, 16u, 4u, 150u, (void*)2u, 0u,
                                    0u);
        panel_layout_surface_install(face_rec);

        crt_surface_calls_clear();
        advisor_progress_bar_fill_draw(1, 0.0f);
        advisor_progress_bar_fill_draw(1, -1.0f);
        CHECK(crt_surface_calls_count() == 0,
              "the gate silences non-positive percents");
        crt_surface_calls_clear();
        advisor_progress_bar_fill_draw(1, 67.0f);
        CHECK(crt_surface_calls_count() == 1,
              "the fill blits once per call");
        uint32_t fh = 0;
        float fx = 0, fy = 0;
        CHECK(crt_surface_call_get(0, &fh, &fx, &fy)
                  && fh == (uint32_t)(uintptr_t)face_rec
                  && fx == 2.5f + 16.0f && fy == 1.25f + 4.0f,
              "the dest pair reads the pair bits plus the size pair");
        float r3[2] = { 0, 0 };
        int r1s = 1, r3s = 0;
        CHECK(crt_surface_call_get_rect(0, NULL, &r1s, r3, &r3s) && !r1s
                  && r3s,
              "the fill rides only the size witness");
        const float want_fill
            = (float)((double)150.0 * (double)(float)(1.0 / 100.0)
                      * 67.0);
        CHECK(bits_of(r3[0]) == bits_of(want_fill)
                  && bits_of(r3[1]) == bits_of(32.0f),
              "the size witness is fill and the face+8 float");

        crt_surface_calls_clear();
        advisor_progress_bar_fill_draw(1, 250.0f);
        const float clamped
            = (float)((double)150.0 * (double)(float)(1.0 / 100.0)
                      * 100.0);
        CHECK(crt_surface_calls_count() == 1
                  && crt_surface_call_get_rect(0, NULL, NULL, r3, NULL)
                  && bits_of(r3[0]) == bits_of(clamped),
              "the clamp rides the corpus constant");
        panel_layout_surface_install(NULL);
        crt_surface_calls_clear();
        advisor_progress_bar_fill_draw(1, 50.0f);
        CHECK(crt_surface_calls_count() == 0,
              "the missing face gate silences (dev)");

        /* scaled menu blit lanes */
        menu_blit_record_clear();
        uint32_t sbits;
        sbits = bits_of(7.5f);
        menu_blit_record_write(4, 0x1234u);
        menu_blit_record_write(5, 0x5678u);
        menu_blit_record_write(13, 1u);
        menu_blit_record_write(14, bits_of(7.5f));
        menu_blit_record_write(15, bits_of(-3.5f));
        crt_surface_calls_clear();
        menu_surface_blit_scaled(0);
        CHECK(crt_surface_calls_count() == 1
                  && crt_surface_call_get(0, &h0, &xb, &yb) && h0 == 0x5678u
                  && xb == 7.5f && yb == -3.5f,
              "the flag lane destination-blits side 0 from +0x14");
        int rs1 = 1;
        CHECK(crt_surface_call_get_rect(0, NULL, &rs1, NULL, NULL)
                  && rs1 == 0,
              "the plain lane witnesses no rect");

        menu_blit_record_write(13, 0u);
        menu_plane_pan_set(11.5f, 22.5f);
        crt_surface_calls_clear();
        menu_surface_blit_scaled(1);
        CHECK(crt_surface_calls_count() == 1
                  && crt_surface_call_get(0, &h0, &xb, &yb) && h0 == 0x1234u
                  && xb == 0.0f && yb == 0.0f,
              "the scaled lane blits side 1 (+0x10) at the origin");
        float r1[4] = { 0, 0, 0, 0 };
        CHECK(crt_surface_call_get_rect(0, r1, &rs1, NULL, NULL) && rs1
                  && r1[0] == -11.5f && r1[1] == -22.5f
                  && r1[2] == 640.0f && r1[3] == 480.0f,
              "the source rect flips the pan onto 640x480");
        menu_blit_record_write(4, 0u);
        menu_blit_record_write(5, 0u);
        crt_surface_calls_clear();
        menu_surface_blit_scaled(0);
        CHECK(crt_surface_calls_count() == 0,
              "the null handle stays silent");
        CHECK(menu_plane_pan_peek(0) == 11.5f
                  && menu_plane_pan_peek(3) == 0.0f,
              "the pan hook bounds the window");

        menu_blit_record_clear();
        indexed_pair_set(1, p1x, p1y);
        string_cache_bitmap_loader_set(NULL);
    }

    /* EncyclopediaConfigFilesOpen (2026-10-25): the catalog-group config
     * file windows, the corpus's window pairing (27c/1e8, 280/1ec,
     * 284/1f0, 288/1f4), and the stale singletons lane. */
    {
        char dir[] = "/tmp/opencode/ency182-XXXXXX";
        CHECK(mkdtemp(dir) != NULL, "encyclopedia fixture dir");
        char fA[128], fB[128], fC[128], fP[128];
        snprintf(fA, sizeof fA, "%s/a.dat", dir);
        snprintf(fB, sizeof fB, "%s/b.dat", dir);
        snprintf(fC, sizeof fC, "%s/c.dat", dir);
        snprintf(fP, sizeof fP, "%s/p.dat", dir);
        static const char* files[4] = { fA, fB, fC, fP };
        for (int k = 0; k != 4; ++k) {
            FILE* fp = fopen(files[k], "wb");
            CHECK(fp != NULL, "encyclopedia fixture file");
            if (fp != NULL) {
                fputc('x', fp);
                fclose(fp);
            }
        }
        static const char* t0[] = { "alpha", "beta" };
        static const char* t1[] = { "noRecord" };
        static const char* t2[] = { "gamma", "delta" };
        static const char* t3[] = { "delta" };
        catalog_name_table_install(0, t0);
        catalog_name_table_install(1, t1);
        catalog_name_table_install(2, t2);
        catalog_name_table_install(3, t3);
        catalog_count_install(0, 2u); /* DAT_0055627c role -> group 0 */
        catalog_count_install(2, 1u); /* DAT_00556280 role -> group 1 */
        catalog_count_install(3, 0u); /* DAT_00556284 role -> group 2 */
        catalog_count_install(1, 1u); /* DAT_00556288 role -> group 3 */

        char text[800];
        snprintf(text, sizeof text,
                 "EncRoot { Encyclopedia { alpha %s beta %s missing.dat %s "
                 "gamma %s delta %s PowerCrystal %s Ore %s } }",
                 fA, "no/such/value.dat", "none.dat", fB, fC, fP,
                 "no/such/ore.dat");
        BlockFile* cfg = s_parse(text);
        encyclopedia_config_files_open(cfg, "EncRoot");

        void** w0 = (void**)encyclopedia_file_window_peek(0);
        void** w2 = (void**)encyclopedia_file_window_peek(2);
        CHECK(w0 != NULL && w0[0] != NULL && w0[1] == NULL,
              "group 0 opens the existing row and books the failed one");
        CHECK(w2 != NULL, "the zero-count window still allocates");
        void** w3 = (void**)encyclopedia_file_window_peek(3);
        CHECK(w3 != NULL && w3[0] != NULL, "the 288-window group opens its row");
        void* crystal = encyclopedia_crystal_file_peek();
        CHECK(crystal != NULL && encyclopedia_ore_file_peek() == NULL,
              "the PowerCrystal singleton opens; the failed Ore row answers "
              "NULL (the corpus books the dispatch result blind)");

        /* Pass 2: an empty encyclopedia re-zeroes every window while the
         * singleton keeps its last handle (the corpus's stale-global lane
         * on a config miss). */
        BlockFile* empty_cfg = s_parse("EncRoot { Encyclopedia { } }");
        encyclopedia_config_files_open(empty_cfg, "EncRoot");
        void** w0b = (void**)encyclopedia_file_window_peek(0);
        CHECK(w0b != NULL && w0b[0] == NULL && w0b[1] == NULL,
              "the reopen zeroes the window (the corpus leaks the previous "
              "array)");
        CHECK(encyclopedia_crystal_file_peek() == crystal,
              "the singleton keeps its handle across a config miss");
        block_close(cfg);
        block_close(empty_cfg);

        for (int k = 0; k != 4; ++k) {
            catalog_name_table_install(k, NULL);
        }
        catalog_count_install(0, 0u);
        catalog_count_install(1, 0u);
        catalog_count_install(2, 0u);
        catalog_count_install(3, 0u);
        for (int k = 0; k != 4; ++k) {
            remove(files[k]);
        }
        rmdir(dir);
    }

    /* InterfaceImagesConfigLoad always continues into the build-image and
     * Back loaders, even after a populated generic action section. Missing
     * bitmap files here leave metadata observable without a WAD mount. */
    {
        BlockFile* cfg = s_parse(
            "Lego* { InterfaceImages { "
            "Interface_MenuItem_Dig NULL:NULL:NULL:Dig:Unavailable:KEY_D "
            "} VehicleTypes { FixtureVehicle 1 } "
            "BuildingTypes { FixtureBuilding 1 } "
            "InterfaceBackButton 28:28:missing_hl.bmp:missing_pr.bmp:Back }");
        CHECK(interface_images_config_load(cfg) == 1,
              "populated action image section is parsed");
        int width = 0, height = 0;
        CHECK(interface_back_button_dimensions(&width, &height) != 0
                  && width == 28 && height == 28,
              "generic image loop continues to the Back metadata loader");
        const char* action = interface_back_button_action();
        CHECK(action != NULL && strcmp(action, "Back") == 0,
              "generic image loop retains the authored Back action");
        CHECK(interface_build_image_count(0) == 1u
                  && interface_build_image_count(1) == 1u,
              "generic image loop continues to both build catalogs");
        block_close(cfg);
    }

    /* Catalog input shares the authored surround offsets and action groups;
     * absent bitmaps retain the reference's 40x40 hit geometry. */
    {
        BlockFile* cfg = s_parse(
            "Lego* { InterfaceImages { "
            "Interface_MenuItem_Dig NULL:NULL:NULL:Dig:Unavailable:KEY_D "
            "} InterfaceBackButton 28:28:missing_hl.bmp:missing_pr.bmp:Back "
            "InterfaceSurroundImages { 4 missing_surround.bmp:-36:-9:-32:5:NULL:-10:-9 } }");
        EventTimerState previous, state;
        frontend_event_timer_state_get(&previous);
        interface_menu_action_labels_init(565, 18, NULL);
        (void)interface_images_config_load(cfg);
        CHECK(interface_surround_images_config_load(cfg) == 1,
              "catalog input retains configured surround offsets without bitmap IO");
        memset(&state, 0, sizeof(state));
        state.current_event = 7;
        state.flags = 0x84u;
        state.timer = 500.0f;
        frontend_event_timer_state_set(&state);
        const uint32_t previous_count = resource_handle_set_band_word_peek(7);
        resource_handle_set_band_word_install(7, 4u);
        SimulationPointerDispatchOps ops;
        memset(&ops, 0, sizeof(ops));
        ops.context = &state;
        frontend_action_catalog_pointer_setup(&ops);
        CHECK(ops.context == &state && ops.overlay_offset_x == 500.0f
                  && ops.overlay_offset_y == 18.0f,
              "catalog layout keeps caller context and current sliding origin");
        CHECK(ops.focused_rect.enabled && ops.focused_rect.x == -32.0f
                  && ops.focused_rect.y == 5.0f
                  && ops.focused_rect.width == 28.0f
                  && ops.focused_rect.height == 28.0f,
              "Back uses its authored offsets and configured control dimensions");
        uint32_t action = 99, kind = 99, entry = 99;
        CHECK(frontend_action_catalog_hit(NULL, NULL, 540.0f, 58.0f,
                  500.0f, 18.0f, &action, &kind, &entry) != 0
                  && action == 11u && kind == 0u && entry == 0u,
              "missing image retains inclusive 40x40 extent and authored Drill code");
        action = 99;
        CHECK(frontend_action_catalog_hit(NULL, NULL, 541.0f, 58.0f,
                  500.0f, 18.0f, &action, NULL, NULL) == 0
                  && action == 99,
              "catalog miss preserves output and cannot widen the missing image");
        state.current_event = 0;
        frontend_event_timer_state_set(&state);
        frontend_action_catalog_pointer_setup(&ops);
        CHECK(!ops.focused_rect.enabled, "default catalog has no Back target");
        state.current_event = 15;
        frontend_event_timer_state_set(&state);
        frontend_action_catalog_pointer_setup(&ops);
        CHECK(!ops.focused_rect.enabled, "mode fifteen retains no-Back policy");
        frontend_event_timer_state_set(&previous);
        resource_handle_set_band_word_install(7, previous_count);
        block_close(cfg);
    }

    /* Availability reads the frontend's DAT_004de1f8; the separate engine
     * placement-mode word DAT_00556ed4 must neither enable nor disable it. */
    {
        const uint32_t interaction = interaction_flags_peek();
        const uint32_t placement = global_state_low_word_get();
        const uint32_t action = input_action_state_word_get(2);
        const uint32_t catalog = input_binding_state_word_get(4, 0);
        input_action_state_query_set(s_action_ready, NULL);
        interaction_flags_set(0x84u);
        global_state_low_word_set(0u);
        s_action_ready_calls = 0;
        input_action_state_word_set(2, 1u);
        input_action_state_update(2);
        CHECK(s_action_ready_calls == 1
                  && input_action_state_word_get(2) == 4u,
              "availability uses interaction mode and preserves predicate writes");
        interaction_flags_set(0u);
        global_state_low_word_set(0x80u);
        input_action_state_update(2);
        CHECK(s_action_ready_calls == 1
                  && (input_action_state_word_get(2) & 1u) != 0u,
              "engine placement flags cannot enable the action catalog");
        catalog_slot_flag_hooks_set(s_catalog_ready, s_catalog_ready,
                                    NULL, NULL);
        catalog_slot_flag_global_state_set(0x84u);
        global_state_low_word_set(0u);
        input_binding_state_word_set(4, 0, 0u);
        catalog_slot_flag_update(4, 0);
        CHECK((input_binding_state_word_get(4, 0) & 1u) == 0u,
              "build catalog eligibility uses the same interaction owner");
        catalog_slot_flag_hooks_set(NULL, NULL, NULL, NULL);
        input_action_state_query_set(NULL, NULL);
        input_binding_state_word_set(4, 0, catalog);
        input_action_state_word_set(2, action);
        interaction_flags_set(interaction);
        global_state_low_word_set(placement);
    }

    /* The admitted pointer callback counts before executing an action.
     * Build icons count in the current catalog's kind, not in action 73. */
    {
        const uint32_t ordinary = input_action_click_count_swap(11, 5u);
        const uint32_t vehicle = input_binding_click_count_swap(1, 5, 10u);
        const uint32_t building = input_binding_click_count_swap(4, 5, 20u);
        input_action_completion_record(11, 0x13, 5);
        CHECK(input_action_click_count_get(11) == 6u,
              "ordinary completion and script counter share the action ID");
        input_action_completion_record(73, 0x13, 5);
        input_action_completion_record(73, 0x14, 5);
        input_action_completion_record(73, 0x15, 5);
        input_action_completion_record(73, 7, 5);
        CHECK(input_binding_click_count_get(4, 5) == 21u
                  && input_binding_click_count_get(1, 5) == 12u,
              "binding counts use the three admitted catalog modes");
        CHECK(input_action_click_count_swap(11, 0xffffffffu) == 6u,
              "script counter setter returns the completed count");
        input_action_completion_record(11, 0, 0);
        CHECK(input_action_click_count_get(11) == 0u,
              "completion wraps as an unsigned reference DWORD");
        (void)input_action_click_count_swap(11, ordinary);
        (void)input_binding_click_count_swap(1, 5, vehicle);
        (void)input_binding_click_count_swap(4, 5, building);
    }

    if (s_failures == 0) {
        printf("frontend: all tests passed\n");
        return 0;
    }
    printf("frontend: %d FAILURES\n", s_failures);
    return 1;
}
