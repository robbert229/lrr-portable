// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_frontend — validation for src/game/frontend.cpp (finding 23).
 *
 * Synthetic fixture mirrors the shipped Lego.cfg shape: root `Lego*` with
 * sibling `Main` (StartLevel/../FrontEnd flag) and `Menu` (MainMenuFull)
 * and `Levels` groups. The fixture exercises:
 *   - the `::` group-separator queries ("Lego* Menu::MainMenuFull …"),
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

#include "game/frontend.hpp"
#include "game/util.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"

static int s_failures = 0;

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

static void test_block_sep(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    CHECK(cfg != NULL, "sep: parse fixture");
    /* the `::` group separator (finding 23, DAT_004aab18) resolves the
     * exe-style query, and the space form still works */
    CHECK(cfg && block_get_strptr(cfg, "Lego* Menu::MainMenuFull MenuCount")
          != NULL, "sep: :: menu path");
    CHECK(cfg && strcmp(block_get_strptr(cfg, "Lego* Menu::MainMenuFull MenuCount"),
                        "4") == 0, "sep: :: menu count");
    CHECK(cfg && strcmp(block_get_strptr(cfg, "Lego* Menu MainMenuFull MenuCount"),
                        "4") == 0, "sep: space menu count");
    CHECK(cfg && strcmp(block_get_strptr(cfg, "Lego* Menu::MainMenuFull Menu1 Item1"),
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

    /* picker grafting: Menu2 = campaign rows, Menu3 = tutorial rows */
    const FeSubmenu* pick = &main->submenus[1];
    CHECK(pick->item_count == 1 + 2, "build: Menu2 = Back + 2 campaign rows");
    CHECK(pick->items[1].level_start == 1, "build: picker row flagged");
    CHECK(strcmp(pick->items[1].target, "Tran01") == 0, "build: row target Tran01");
    CHECK(strcmp(pick->items[1].label, "1._Moving_Rock_Raiders") == 0,
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

static FeInput s_small = { 0, 0, 0, 0 };

static void test_nav_and_actions(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    CHECK(frontend_open(&fe, FE_MENU_MAIN), "nav: open");
    CHECK(fe.sub == 0 && fe.sel == 0, "nav: starts at Menu1 sel 0");

    FeInput in = { 0, 0, 0, 0 };
    in.down = 1;
    CHECK(frontend_tick(&fe, &in) == FE_EVT_NONE && fe.sel == 1, "nav: down -> 1");
    in.down = 0; in.down = 1; in.enter = 0;
    /* go to sel 3 = Quit_Game */
    frontend_tick(&fe, &in);
    frontend_tick(&fe, &in);
    CHECK(fe.sel == 3, "nav: down x3 -> Quit_Game");
    in.down = 0; in.enter = 1;
    CHECK(frontend_tick(&fe, &in) == FE_EVT_NONE && fe.sub == 3,
        "nav: Quit_Game -> confirm submenu");
    in.enter = 0;
    /* No -> back to Menu1 */
    in.down = 1;
    frontend_tick(&fe, &in);
    CHECK(fe.sel == 1, "nav: confirm sel -> No");
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in);
    CHECK(fe.sub == 0, "nav: No -> Menu1");

    /* start a level: Menu1 Start_Game -> Menu2 picker -> row 1 (Tran01) */
    in.enter = 0;
    frontend_open(&fe, FE_MENU_MAIN);          /* reset to root */
    in.enter = 1;
    CHECK(frontend_tick(&fe, &in) == FE_EVT_NONE && fe.sub == 1,
        "nav: Start_Game -> picker");
    in.enter = 0; in.down = 1;
    frontend_tick(&fe, &in);
    CHECK(fe.sel == 1, "nav: picker sel -> Tran01(row1)");
    in.down = 0; in.enter = 1;
    int ev = (int)frontend_tick(&fe, &in);
    CHECK(ev == (int)FE_EVT_START_LEVEL, "nav: Enter on row -> START_LEVEL");
    CHECK(strcmp(fe.pending_level, "Tran01") == 0, "nav: pending_level Tran01");
    CHECK(!fe.open, "nav: menu closed after start");

    /* quit flow: root -> Quit_Game -> Yes */
    frontend_open(&fe, FE_MENU_MAIN);
    in.enter = 0; in.down = 1;
    FeInput tmp = { 0, 1, 0, 0 };
    frontend_tick(&fe, &tmp);
    frontend_tick(&fe, &tmp);
    frontend_tick(&fe, &tmp);
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in);                   /* -> confirm */
    in.enter = 0; in.enter = 1;
    ev = (int)frontend_tick(&fe, &in);
    CHECK(ev == (int)FE_EVT_QUIT, "nav: Yes -> QUIT");
    CHECK(fe.quit, "nav: quit flag set");

    /* esc: back then quit */
    frontend_open(&fe, FE_MENU_MAIN);
    in.enter = 0; in.down = 1;
    FeInput down_ = { 0, 1, 0, 0 };
    frontend_tick(&fe, &down_);
    frontend_tick(&fe, &down_);
    frontend_tick(&fe, &down_);
    in = s_small; in.enter = 1;
    frontend_tick(&fe, &in);      /* -> confirm */
    in.enter = 0; in.esc = 1;
    CHECK(frontend_tick(&fe, &in) == FE_EVT_NONE && fe.sub == 0,
        "nav: esc on confirm -> Menu1");
    in.esc = 0;
    FeInput esc_ = { 0, 0, 0, 1 };
    CHECK(frontend_tick(&fe, &esc_) == FE_EVT_QUIT,
        "nav: esc on root -> QUIT");
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

    int w = 320, h = 240;
    uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
    CHECK(px != NULL, "render: buffer");
    /* clear to black explicitly for deterministic assertions */
    memset(px, 0, (size_t)w * h * 4);
    frontend_render(&fe, w, h, px, w * 4);
    uint8_t b[4];

    /* selected row 0 = "Start_Game": auto-centered, len 10 -> tw 70,
     * x = 160 - 35 = 125, y = 56. Highlight bar behind it. */
    b[0] = px[(54 * w + 121) * 4 + 0];
    b[1] = px[(54 * w + 121) * 4 + 1];
    b[2] = px[(54 * w + 121) * 4 + 2];
    CHECK(b[0] == 60 && b[1] == 80 && b[2] == 200,
        "render: selection highlight color");
    /* 'S' of Start_Game at (125,56) is ink (white) */
    b[0] = px[(56 * w + 125) * 4 + 0];
    CHECK(b[0] == 255 && px[(56 * w + 125) * 4 + 1] == 255
        && px[(56 * w + 125) * 4 + 2] == 255, "render: glyph ink");

    /* non-selected row 3 "Quit_Game" (len 9 -> 63, x = 160 - 63/2 = 129),
     * y = 56 + 3*40 = 176; faint color (200,200,160). */
    b[0] = px[(176 * w + 129) * 4 + 0];
    CHECK(b[0] == 200 && px[(176 * w + 129) * 4 + 1] == 200
        && px[(176 * w + 129) * 4 + 2] == 160, "render: faint row ink");

    free(px);
    block_close(cfg);
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
    if (cfg != NULL) {
        block_close(cfg);
    }
}

int main(int argc, char** argv)
{
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            real_probe(argv[a + 1]);
        }
    }

    test_block_sep();
    test_build();
    test_gate();
    test_nav_and_actions();
    test_render();

    if (s_failures == 0) {
        printf("frontend: all tests passed\n");
        return 0;
    }
    printf("frontend: %d FAILURES\n", s_failures);
    return 1;
}