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
#include <unistd.h> /* mkdtemp / rmdir */

#include "game/frontend.hpp"
#include "game/util.hpp"
#include "game/game.hpp"
#include "game/select.hpp"
#include "gods98/block.hpp"
#include "gods98/message.hpp"
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

static FeInput s_small = { 0, 0, 0, 0, 0, 0, -1, -1, 0 };

static void test_nav_and_actions(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    FrontEnd fe;
    frontend_build(&fe, cfg);
    CHECK(frontend_open(&fe, FE_MENU_MAIN), "nav: open");
    CHECK(fe.sub == 0 && fe.sel == 0, "nav: starts at Menu1 sel 0");

    /* Overlay players open lazily on the first tick of the active submenu
     * (PlayRandom picks one; no fixture files, so the flic stays NULL but
     * the attempted flag is set — no per-frame retries). */
    FeInput ov = { 0, 0, 0, 0, 0, 0, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &ov, NULL) == FE_EVT_NONE, "nav: overlay tick ok");
    CHECK(fe.menus[FE_MENU_MAIN].submenus[0].overlays[0].tried == 1,
          "nav: overlay open attempted");
    CHECK(fe.menus[FE_MENU_MAIN].submenus[0].overlays[0].flic == NULL,
          "nav: missing overlay file -> no flic (no crash)");
    CHECK(fe.menus[FE_MENU_MAIN].submenus[0].overlays[0].tried == 1,
          "nav: overlay open not retried every frame");

    FeInput in = { 0, 0, 0, 0, 0, 0, -1, -1, 0 };
    in.down = 1;
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_NONE && fe.sel == 1, "nav: down -> 1");
    in.down = 0; in.down = 1; in.enter = 0;
    /* go to sel 3 = Quit_Game */
    frontend_tick(&fe, &in, NULL);
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sel == 3, "nav: down x3 -> Quit_Game");
    in.down = 0; in.enter = 1;
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_NONE && fe.sub == 3,
        "nav: Quit_Game -> confirm submenu");
    in.enter = 0;
    /* No -> back to Menu1 */
    in.down = 1;
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sel == 1, "nav: confirm sel -> No");
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sub == 0, "nav: No -> Menu1");

    /* start a level: Menu1 Start_Game -> Menu2 picker -> row 1 (Tran01) */
    in.enter = 0;
    frontend_open(&fe, FE_MENU_MAIN);          /* reset to root */
    in.enter = 1;
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_NONE && fe.sub == 1,
        "nav: Start_Game -> picker");
    in.enter = 0; in.down = 1;
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sel == 1, "nav: picker sel -> Tran01(row1)");
    in.down = 0; in.enter = 1;
    int ev = (int)frontend_tick(&fe, &in, NULL);
    CHECK(ev == (int)FE_EVT_START_LEVEL, "nav: Enter on row -> START_LEVEL");
    CHECK(strcmp(fe.pending_level, "Tran01") == 0, "nav: pending_level Tran01");
    CHECK(!fe.open, "nav: menu closed after start");

    /* quit flow: root -> Quit_Game -> Yes */
    frontend_open(&fe, FE_MENU_MAIN);
    in.enter = 0; in.down = 1;
    FeInput tmp = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    frontend_tick(&fe, &tmp, NULL);
    frontend_tick(&fe, &tmp, NULL);
    frontend_tick(&fe, &tmp, NULL);
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in, NULL);                   /* -> confirm */
    in.enter = 0; in.enter = 1;
    ev = (int)frontend_tick(&fe, &in, NULL);
    CHECK(ev == (int)FE_EVT_QUIT, "nav: Yes -> QUIT");
    CHECK(fe.quit, "nav: quit flag set");

    /* esc: back then quit */
    frontend_open(&fe, FE_MENU_MAIN);
    in.enter = 0; in.down = 1;
    FeInput down_ = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    frontend_tick(&fe, &down_, NULL);
    frontend_tick(&fe, &down_, NULL);
    frontend_tick(&fe, &down_, NULL);
    in = s_small; in.enter = 1;
    frontend_tick(&fe, &in, NULL);      /* -> confirm */
    in.enter = 0; in.esc = 1;
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_NONE && fe.sub == 0,
        "nav: esc on confirm -> Menu1");
    in.esc = 0;
    FeInput esc_ = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &esc_, NULL) == FE_EVT_QUIT,
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
    CHECK(b[0] == 60 && b[1] == 80 && b[2] == 200,
        "render: selection highlight color");
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
            /* advance('L') = maxx+1 = 2 -> 'Q' at x = 4 + 2 = 6, ink at
             * (6+1, 5) = (7,5) */
            uint8_t q0 = px[(5 * w + 7) * 4 + 0];
            CHECK(q0 == 255, "font: Q advance ink");
            /* the key pixel of the 'L' cell (its 0,0) is transparent: the
             * selected-row highlight bar behind it shows through (not the
             * red key, not white ink) */
            uint8_t k = px[(4 * w + 4) * 4 + 0];
            CHECK(k == 60 && px[(4 * w + 4) * 4 + 1] == 80
                && px[(4 * w + 4) * 4 + 2] == 200,
                "font: key skipped (highlight shows through)");
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

    /* Enter on Start_Game (item 0) -> Menu2 (Missions picker) */
    FeInput in = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    frontend_tick(&fe, &in, NULL);
    CHECK(fe.sub == 1, "img: navigated to Menu2");
    const char* im = frontend_active_image(&fe);
    CHECK(im != NULL, "img: Menu2 has a MenuImage");
    CHECK(im != NULL && strcmp(im, "Interface\\LevelPicker\\LevelPick.bmp") == 0,
        "img: Menu2 path trimmed of ':0:0:1'");
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
    CHECK(frontend_tick(&fe, &miss, NULL) == FE_EVT_NONE && fe.sel == 0
        && fe.sub == 0, "ptr: empty-space click is a no-op");

    /* click the selected Start_Game (item 0: x=285, y=200) -> Missions */
    FeInput start = { 0, 0, 0, 0, 0, 0, 300, 205, 1 };
    CHECK(frontend_tick(&fe, &start, NULL) == FE_EVT_NONE && fe.sub == 1,
        "ptr: click Start_Game -> Missions picker");
    block_close(cfg);
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

    /* Continue_Game (Trigger flag 1) resumes instead of quitting. */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput enter = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &enter, NULL) == FE_EVT_RESUME && !fe.open,
        "pause: Continue -> RESUME");

    /* Esc on the pause root resumes. */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput esc = { 0, 0, 0, 0, 0, 1, -1, -1, 0 };
    CHECK(frontend_tick(&fe, &esc, NULL) == FE_EVT_RESUME && !fe.open,
        "pause: Esc on root -> RESUME");

    /* Quit = item 4 (sel 3) -> confirm (Menu3) -> Yes -> QUIT. */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput in = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    frontend_tick(&fe, &in, NULL);            /* sel 1 */
    frontend_tick(&fe, &in, NULL);            /* sel 2 */
    frontend_tick(&fe, &in, NULL);            /* sel 3 (Quit) */
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in, NULL);            /* navigate -> Menu3 */
    CHECK(fe.sub == 2 && fe.sel == 0,
        "pause: Quit -> confirm submenu");
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_QUIT,
        "pause: Yes on quit confirm -> QUIT");

    /* Restart = item 3 (sel 2) -> confirm (Menu4) -> Yes -> START_LEVEL. */
    frontend_open(&fe, FE_MENU_PAUSED);
    snprintf(fe.resume_level, FE_LEVEL_NAME_MAX, "%s", "Tran01");
    in.down = 1; in.enter = 0;
    frontend_tick(&fe, &in, NULL);            /* sel 1 */
    frontend_tick(&fe, &in, NULL);            /* sel 2 (Restart_Mission) */
    in.down = 0; in.enter = 1;
    frontend_tick(&fe, &in, NULL);            /* navigate -> Menu4 */
    CHECK(fe.sub == 3 && fe.sel == 0,
        "pause: Restart_Mission -> confirm submenu");
    CHECK(frontend_tick(&fe, &in, NULL) == FE_EVT_START_LEVEL
        && strcmp(fe.pending_level, "Tran01") == 0,
        "pause: Yes on restart -> START_LEVEL(resume level)");

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
    snprintf(slots[0].label, FE_NAME_MAX, "%s", "Slot 1 (empty)");
    slots[0].id = 1; slots[0].present = 0;
    snprintf(slots[1].label, FE_NAME_MAX, "%s", "Slot 2");
    slots[1].id = 2; slots[1].present = 1;
    snprintf(slots[2].label, FE_NAME_MAX, "%s", "Slot 3");
    slots[2].id = 3; slots[2].present = 1;

    CHECK(frontend_build_slots(&fe, slots, 3) == 1, "slots: grafted");
    CHECK(fe.slot_count == 3, "slots: registered count");
    const FeMenu* m = frontend_menu(&fe, FE_MENU_MAIN);
    CHECK(m->submenus[1].item_count == 1 + 3,
        "slots: 3 rows appended on the SaveLoad screen");
    CHECK(m->submenus[1].items[1].load_slot == 1,
        "slots: row 1 binds slot 1");
    CHECK(strcmp(m->submenus[1].items[2].label, "Slot 2") == 0,
        "slots: row 2 label");

    frontend_open(&fe, FE_MENU_MAIN);
    FeInput ent = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    frontend_tick(&fe, &ent, NULL);           /* Menu1 item (Load) -> Menu2 */
    CHECK(fe.sub == 1, "slots: navigated to SaveLoad screen");
    FeInput down = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    frontend_tick(&fe, &down, NULL);          /* sel 1 = first slot row */
    ent.down = 0; ent.enter = 1;
    CHECK(frontend_tick(&fe, &ent, NULL) == FE_EVT_LOAD_GAME
        && fe.load_slot == 1, "slots: picking slot 1 -> LOAD_GAME");

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
    FeInput ein = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    frontend_tick(&fe, &ein, NULL);
    CHECK(fe.sub == 1, "opt: on the Options submenu");

    FeItem* sl = &fe.menus[FE_MENU_PAUSED].submenus[1].items[0];
    CHECK(sl->kind == FE_ITEM_SLIDER && sl->min == 0 && sl->max == 10
        && sl->value == 8, "opt: slider parsed with default 8");

    FeInput r = { 0, 0, 0, 1, 0, 0, -1, -1, 0 };   /* right */
    fe.option_changed = 0;
    frontend_tick(&fe, &r, NULL);
    CHECK(sl->value == 9 && fe.option_changed == 1, "opt: right increments");
    FeInput l = { 0, 0, 1, 0, 0, 0, -1, -1, 0 };   /* left */
    frontend_tick(&fe, &l, NULL);
    frontend_tick(&fe, &l, NULL);
    CHECK(sl->value == 7, "opt: left decrements (9 -> 7)");

    /* Cycle item */
    FeInput down = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    frontend_tick(&fe, &down, NULL);
    FeItem* cy = &fe.menus[FE_MENU_PAUSED].submenus[1].items[1];
    CHECK(cy->kind == FE_ITEM_CYCLE && cy->cycle_count == 2
        && cy->value == 0 && strcmp(cy->cycle_opts[1], "On") == 0,
        "opt: cycle parsed with options");
    frontend_tick(&fe, &r, NULL);
    CHECK(cy->value == 1, "opt: cycle right -> option 1");
    frontend_tick(&fe, &r, NULL);
    CHECK(cy->value == 1, "opt: cycle clamps at max");

    /* Render the slider widget (3x5 fallback): item0 at Position 250:134,
     * label "SFX_Volume" tw 70 -> bar x=250+80=330, y=134+6-2=138, width 72;
     * value 7 -> fill 7*72/10=50 => gold 330..379, dark 380..401. */
    int w = 480, h = 320;
    uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
    CHECK(px != NULL, "opt: buffer");
    if (px != NULL) {
        memset(px, 0, (size_t)w * h * 4);
        frontend_render(&fe, NULL, NULL, w, h, px, w * 4);
        uint8_t c[3];
        c[0] = px[(140 * w + 334) * 4 + 0];
        c[1] = px[(140 * w + 334) * 4 + 1];
        c[2] = px[(140 * w + 334) * 4 + 2];
        CHECK(c[0] == 210 && c[1] == 180 && c[2] == 70,
            "opt: bar fill is gold");
        c[0] = px[(140 * w + 384) * 4 + 0];
        c[1] = px[(140 * w + 384) * 4 + 1];
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
        "\t\tLv0 { FullName Lv0 NextLevel Levels::Lv1 "
        "FrontEndX 300 FrontEndY 100 }\n"
        "\t\tLv1 { FullName Lv1 NextLevel Levels::Lv2 "
        "FrontEndX 300 FrontEndY 900 }\n"
        "\t\tLv2 { FullName Lv2 FrontEndX 300 FrontEndY 1800 }\n"
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

    FeInput enter = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    frontend_tick(&fe, &enter, NULL);
    CHECK(fe.sub == 1, "scr: into the picker sub");

    FeInput mid = { 0, 0, 0, 0, 0, 0, 320, 240, 0 };
    frontend_tick(&fe, &mid, NULL);
    CHECK(pk->scroll == 0, "scr: deadzone stays put");

    /* bottom band: 50 px/tick down to the content clamp 480-1816 = -1336 */
    FeInput bot = { 0, 0, 0, 0, 0, 0, 320, 460, 0 };
    frontend_tick(&fe, &bot, NULL);
    CHECK(pk->scroll == -50, "scr: bottom band pans");
    for (int i = 0; i < 40 && pk->scroll > -1336; i++) {
        frontend_tick(&fe, &bot, NULL);
    }
    CHECK(pk->scroll == -1336, "scr: clamps at the plane's bottom");

    /* top band pans back up */
    FeInput top = { 0, 0, 0, 0, 0, 0, 320, 20, 0 };
    frontend_tick(&fe, &top, NULL);
    CHECK(pk->scroll == -1286, "scr: top band pans back");

    /* rows off the plane are gone; the visible bottom row starts on click
     * (Lv2 sits at 1800-1336=464; that y also clamps the scroll back) */
    FeInput cl = { 0, 0, 0, 0, 0, 0, 310, 464, 1 };
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

    /* tutorials occupy records 0..tutorial_n-1, campaign follows */
    CHECK(fe_record_for_level(&fe, "MoveTuto01") == 0, "rec: tutorial = record 0");
    CHECK(fe_record_for_level(&fe, "Tran01") == 1, "rec: campaign starts after tutorials");
    CHECK(fe_record_for_level(&fe, "Tran02") == 2, "rec: chain position = record");
    CHECK(fe_record_for_level(&fe, "Nowhere") == -1, "rec: unknown -> -1");
    CHECK(fe_record_for_level(&fe, "") == -1, "rec: empty -> -1");

    char name[FE_LEVEL_NAME_MAX];
    name[0] = '\0';
    CHECK(fe_level_for_record(&fe, 0, name, sizeof name)
              && strcmp(name, "MoveTuto01") == 0,
        "rec: inverse maps the tutorial");
    CHECK(fe_level_for_record(&fe, 2, name, sizeof name)
              && strcmp(name, "Tran02") == 0,
        "rec: inverse maps the campaign tail");
    CHECK(fe_level_for_record(&fe, 77, name, sizeof name) == 0,
        "rec: out-of-range inverse fails clean");
    block_close(cfg);
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
    test_active_image();
    test_font();
    test_pointer();
    test_pause();
    test_slots();
    test_options();
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
        timed_effect_update_request_set(16); /* outside the window */
        CHECK(timed_effect_flags_peek(16) == 0u, "the guarded window reads zero");

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

        selection_region_parameters_set(1.0f, 2.0f, 3.0f, 4.0f, 10.0f);
        float reg[5];
        selection_region_parameters_peek(reg);
        CHECK(reg[0] == 1.0f && reg[1] == 2.0f && reg[2] == 3.0f
                  && reg[3] == 4.0f && reg[4] == 0.4f,
              "the region parameters land scaled by 0.04");

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

    if (s_failures == 0) {
        printf("frontend: all tests passed\n");
        return 0;
    }
    printf("frontend: %d FAILURES\n", s_failures);
    return 1;
}