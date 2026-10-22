// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_options — functional Options/Advanced controls (issue #85).
 *
 * Covers the four acceptance axes:
 *   1. store semantics — documented defaults, shipped ranges, clamping;
 *   2. menu-to-runtime — keyboard adjustments drive the live store, and the
 *      store drives the runtime consumers (Game Speed -> the tick's time
 *      scale, Brightness -> the rendered frame, Help Window -> the overlay);
 *   3. effective-value sync — re-entering Options (or using the *other*
 *      Options instance, Paused vs OptionsMenu) shows what was changed,
 *      never a reset to default;
 *   4. save-context persistence — values ride the `.sav` slot header
 *      round trip, with the defaults on a fresh game and on an un-stamped
 *      legacy save.
 *
 * With `--real <wad-dir>` the shipped Lego.cfg's OptionsMenu is rebuilt from
 * the real wad and its control names + ranges are asserted (the config the
 * store's ids mirror).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/frontend.hpp"
#include "game/game.hpp"
#include "game/level.hpp"
#include "game/objective.hpp"
#include "game/options.hpp"
#include "game/render.hpp"
#include "game/savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
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

/* Fixture: the shipped Options screen's shape — Game_Speed / SFX / Music /
 * Brightness sliders + the Help_Window cycle — in the OptionsMenu group,
 * plus a PausedMenu whose Options submenu carries Game_Speed + Brightness
 * (the second Options instance). */
static const char* FIXCFG =
    "Lego* {\n"
    "\tMain { StartLevel Levels::Tran01 FrontEnd TRUE }\n"
    "\tMenu {\n"
    "\t\tMainMenuFull {\n"
    "\t\t\tMenuCount 1\n"
    "\t\t\tMenu1 { AutoCenter TRUE Position 320:200 ItemCount 1\n"
    "\t\t\t\tItem1 Next:0:0:Start_Game:Menu2 }\n"
    "\t\t}\n"
    "\t\tOptionsMenu {\n"
    "\t\t\tMenuCount 1\n"
    "\t\t\tMenu1 { AutoCenter FALSE Position 216:134 ItemCount 5\n"
    "\t\t\t\tItem1 Slider:-80:30:255:0:Game_Speed:0:5:bar.bmp\n"
    "\t\t\t\tItem2 Slider:-80:55:255:0:SFX_Volume:0:10:bar.bmp\n"
    "\t\t\t\tItem3 Slider:-80:80:255:0:Music_Volume:0:10:bar.bmp\n"
    "\t\t\t\tItem4 Slider:-80:105:255:0:Brightness:0:10:bar.bmp\n"
    "\t\t\t\tItem5 Cycle:-80:130:255:0:Help_Window:2:Off:On\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t\tPausedMenu {\n"
    "\t\t\tMenuCount 2\n"
    "\t\t\tMenu1 { Title Paused Position 250:134 ItemCount 1\n"
    "\t\t\t\tItem1 Next:60:80:Options:Menu2 }\n"
    "\t\t\tMenu2 { Title Options AutoCenter FALSE Position 250:134\n"
    "\t\t\t\tItemCount 3\n"
    "\t\t\t\tItem1 Slider:-80:30:255:0:Game_Speed:0:5:bar.bmp\n"
    "\t\t\t\tItem2 Slider:-80:55:255:0:Brightness:0:10:bar.bmp\n"
    "\t\t\t\tItem3 Next:-80:180:Back:Menu1 }\n"
    "\t\t}\n"
    "\t}\n"
    "\tLevels { Tran01 { FullName T } }\n"
    "}\n";

/* --- 1. store semantics ---------------------------------------------------- */

static void test_store(void)
{
    GameOptions o;
    options_defaults(&o);
    CHECK(o.game_speed == 1 && o.sfx_volume == 8 && o.music_volume == 5
        && o.brightness == 8 && o.help_window == 0,
        "store: corpus defaults (OptionsLoadOrDefault triple + port neutrals)");

    CHECK(options_id_for_label("Game_Speed") == OPT_GAME_SPEED
        && options_id_for_label("game speed") == OPT_GAME_SPEED
        && options_id_for_label("Brightness") == OPT_BRIGHTNESS
        && options_id_for_label("Help_Window") == OPT_HELP_WINDOW
        && options_id_for_label("SFX_Volume") == OPT_SFX_VOLUME
        && options_id_for_label("Music_Volume") == OPT_MUSIC_VOLUME
        && options_id_for_label("Start_Game") == -1,
        "store: banner->id mapping");

    int lo = 0, hi = 0;
    CHECK(options_range(OPT_GAME_SPEED, &lo, &hi) && lo == 0 && hi == 5
        && options_range(OPT_SFX_VOLUME, &lo, &hi) && hi == 10
        && options_range(OPT_MUSIC_VOLUME, &lo, &hi) && hi == 10
        && options_range(OPT_BRIGHTNESS, &lo, &hi) && hi == 10
        && options_range(OPT_HELP_WINDOW, &lo, &hi) && hi == 1,
        "store: shipped config ranges");

    options_set(&o, OPT_GAME_SPEED, 99);
    CHECK(o.game_speed == 5, "store: set clamps to cfg hi");
    options_set(&o, OPT_GAME_SPEED, -9);
    CHECK(o.game_speed == 0, "store: set clamps to cfg lo");

    /* time scale: default index 1 = normal, table rises to 2.0 at 5. */
    options_defaults(&o);
    CHECK(options_time_scale(&o) == 1.0f, "store: default speed = 1.0x");
    options_set(&o, OPT_GAME_SPEED, 5);
    CHECK(options_time_scale(&o) > 1.5f, "store: speed 5 scales up");
    options_set(&o, OPT_GAME_SPEED, 0);
    CHECK(options_time_scale(&o) < 1.0f, "store: speed 0 scales down");
}

/* --- 2 + 3. menu -> runtime, effective-value sync --------------------------- */

static void test_menu_to_runtime(void)
{
    BlockFile* cfg = s_parse(FIXCFG);
    CHECK(cfg != NULL, "menu: fixture parses");

    FrontEnd fe;
    GameOptions store;
    options_defaults(&store);
    CHECK(frontend_build(&fe, cfg) == 1, "menu: builds");
    fe.opts = &store;
    frontend_options_sync(&fe);

    const FeSubmenu* sub = &fe.menus[FE_MENU_OPTIONS].submenus[0];
    const FeItem* speed = &sub->items[0];
    const FeItem* bright = &sub->items[3];
    const FeItem* helpw = &sub->items[4];
    CHECK(speed->min == 0 && speed->max == 5 && speed->value == 1,
        "menu: speed seeded effective default 1");
    CHECK(bright->value == 8 && helpw->value == 0,
        "menu: bright/help seeded");

    /* Open Options and drive it like the shipped keyboard: speed +1. */
    frontend_open(&fe, FE_MENU_OPTIONS);
    CHECK(fe.menus[FE_MENU_OPTIONS].submenus[0].items[0].value == 1,
        "menu: open seeds the effective value");
    FeInput r = { 0, 0, 0, 1, 0, 0, -1, -1, 0 };   /* right */
    fe.option_changed = 0;
    frontend_tick(&fe, &r, NULL);
    CHECK(store.game_speed == 2 && fe.option_changed == 1,
        "menu->store: speed right increments the store");
    CHECK(options_time_scale(&store) == 1.25f,
        "store->runtime: EngineTick dt factor follows the slider");

    /* Brightness: walk down to item 3 and crank past the cfg hi -> clamp 10. */
    FeInput d = { 0, 1, 0, 0, 0, 0, -1, -1, 0 };
    for (int i = 0; i < 3; i++) {
        frontend_tick(&fe, &d, NULL);
    }
    CHECK(frontend_active_item(&fe) == bright, "menu: selected Brightness row");
    for (int i = 0; i < 6; i++) {
        frontend_tick(&fe, &r, NULL);
    }
    CHECK(store.brightness == 10 && bright->value == 10,
        "menu->store: brightness clamps at cfg hi 10");

    /* Help_Window cycle Off -> On. */
    frontend_tick(&fe, &d, NULL);
    CHECK(frontend_active_item(&fe) == helpw, "menu: selected Help_Window row");
    frontend_tick(&fe, &r, NULL);
    CHECK(store.help_window == 1 && helpw->value == 1,
        "menu->store: help cycle writes the store");
    /* Clamps at the cycle's option count too. */
    frontend_tick(&fe, &r, NULL);
    CHECK(store.help_window == 1, "menu->store: help clamps at On");

    /* Brightness's runtime effect on the rendered frame: the store value is
     * what the frame loop applies, so apply it here. A full-bright frame at
     * value 10 is untouched; at value 0 it goes black. */
    int w = 64, h = 32;
    uint8_t* px = (uint8_t*)calloc((size_t)w * h, 4);
    CHECK(px != NULL, "effect: framebuffer");
    if (px != NULL) {
        memset(px, 200, (size_t)w * h * 4);
        render_apply_brightness(px, w * 4, w, h, store.brightness);
        CHECK(px[0] == 200, "effect: brightness 10 leaves the frame intact");
        memset(px, 200, (size_t)w * h * 4);
        render_apply_brightness(px, w * 4, w, h, 0);
        CHECK(px[0] == 0, "effect: brightness 0 renders black");
        memset(px, 200, (size_t)w * h * 4);
        render_apply_brightness(px, w * 4, w, h, 5);
        CHECK(px[0] == 100 || px[0] == 99, "effect: half value halves pixels");

        /* Help Window's in-game display: the production gate lives in
         * frontend_help_overlay itself (frameloop calls it unconditionally),
         * so drive both store states through it. */
        memset(px, 0, (size_t)w * h * 4);
        store.help_window = 0;
        frontend_help_overlay(px, w * 4, w, h, "TEST", &store);
        int touched = 0;
        for (int i = 0; i < w * h * 4; i++) {
            if (px[i] != 0) {
                touched = 1;
                break;
            }
        }
        CHECK(touched == 0, "effect: Help_Window Off draws nothing");
        memset(px, 0, (size_t)w * h * 4);
        store.help_window = 1;
        frontend_help_overlay(px, w * 4, w, h, "TEST", &store);
        touched = 0;
        for (int i = 0; i < w * h * 4; i++) {
            if (px[i] != 0) {
                touched = 1;
                break;
            }
        }
        CHECK(touched == 1, "effect: Help_Window On draws the panel");
        store.help_window = 1;
        free(px);
    }

    /* Effective-value sync: close and re-open — nothing resets to default. */
    frontend_close(&fe);
    frontend_open(&fe, FE_MENU_OPTIONS);
    CHECK(fe.menus[FE_MENU_OPTIONS].submenus[0].items[0].value == 2
        && fe.menus[FE_MENU_OPTIONS].submenus[0].items[3].value == 10
        && fe.menus[FE_MENU_OPTIONS].submenus[0].items[4].value == 1,
        "sync: re-entry shows the effective values");

    /* The other Options instance (PausedMenu's submenu) is seeded too. */
    frontend_open(&fe, FE_MENU_PAUSED);
    FeInput ent = { 0, 0, 0, 0, 1, 0, -1, -1, 0 };
    frontend_tick(&fe, &ent, NULL);             /* Options:Menu2 */
    CHECK(fe.sub == 1, "sync: pause navigated into Options");
    CHECK(fe.menus[FE_MENU_PAUSED].submenus[1].items[0].value == 2
        && fe.menus[FE_MENU_PAUSED].submenus[1].items[1].value == 10,
        "sync: Paused options show the same effective values");

    /* A store change from outside (e.g. the slot load) re-seeds on re-entry. */
    store.game_speed = 4;
    frontend_open(&fe, FE_MENU_OPTIONS);
    CHECK(fe.menus[FE_MENU_OPTIONS].submenus[0].items[0].value == 4,
        "sync: external store change shows on re-entry");

    block_close(cfg);
}

/* --- production speed connection: store -> engine (PR #88 review) ----------
 * Exercises the exact calls the frame loop makes per frame:
 * options_scale_frame stamps the store's factor into an EngineContext and
 * returns the scaled dt; EngineTick reads the stamp back through
 * EngineTimeScale. Dropping either half changes what these see. */
static void test_speed_connection(void)
{
    EngineContext ctx = { 0 };
    GameOptions store;
    options_defaults(&store);

    float sdt = options_scale_frame(&store, &ctx, 1.0f);
    CHECK(sdt == 1.0f && EngineTimeScale(&ctx) == 1.0f,
        "speed: default store scales the frame 1:1");
    options_set(&store, OPT_GAME_SPEED, 5);
    sdt = options_scale_frame(&store, &ctx, 0.5f);
    CHECK(sdt == 1.0f && EngineTimeScale(&ctx) == 2.0f,
        "speed: slider 5 doubles ctx + ticker dt");
    options_set(&store, OPT_GAME_SPEED, 0);
    sdt = options_scale_frame(&store, &ctx, 0.5f);
    CHECK(sdt == 0.25f && EngineTimeScale(&ctx) == 0.5f,
        "speed: slider 0 halves ctx + ticker dt");

    /* Observable pacing: the frame loop feeds objective_tick the scaled dt,
     * and the corpus timer objective ticks down on exactly that. (An unmet
     * crystal gate keeps the goal live so the timer keeps draining.) */
    LevelState* s = (LevelState*)calloc(1, sizeof(LevelState));
    CHECK(s != NULL, "speed: level state");
    if (s != NULL) {
        s->objective = LVL_OBJECTIVE_TIMER | LVL_OBJECTIVE_CRYSTALS;
        level_set_int(s, OBJ_OFF_CRYSTAL_LIVE, 0);
        level_set_int(s, OBJ_OFF_CRYSTAL_TARGET, 1);
        level_set_float(s, OBJ_OFF_TIMER_TARGET, 10.0f);

        options_set(&store, OPT_GAME_SPEED, 1);   /* 1.0x */
        for (int i = 0; i < 10; i++) {
            objective_tick(s, options_scale_frame(&store, &ctx, 0.1f));
        }
        float slow = level_float(s, OBJ_OFF_TIMER_TARGET);
        options_set(&store, OPT_GAME_SPEED, 5);   /* 2.0x */
        for (int i = 0; i < 10; i++) {
            objective_tick(s, options_scale_frame(&store, &ctx, 0.1f));
        }
        float fast = level_float(s, OBJ_OFF_TIMER_TARGET);
        /* 10 frames at 0.1 * 1.0 drained 1.0; the 2.0x stretch drained 2.0. */
        CHECK(slow > 8.9f && slow < 9.1f,
            "speed: normal pace drains the objective timer at 1.0x");
        CHECK(fast > 6.9f && fast < 7.1f,
            "speed: slider 5 drains the objective timer 2x");
        free(s);
    }
}

/* --- restored audio takes effect at once (PR #88 P2) ------------------------
 * The frame loop opens audio with options_audio_gain instead of a fixed
 * 0.6, so the .sav-restored store drives the live player directly. */
static void test_audio_restoration(void)
{
    GameOptions store;
    options_defaults(&store);
    AudioPlayer ap = { 0 };
    audio_set_volume(&ap, options_audio_gain(&store));
    CHECK(ap.volume > 0.79f && ap.volume < 0.81f,
        "audio: default SFX 8 opens the player at 0.8");

    options_set(&store, OPT_SFX_VOLUME, 0);
    audio_set_volume(&ap, options_audio_gain(&store));
    CHECK(ap.volume == 0.0f, "audio: saved zero volume opens silent");

    options_set(&store, OPT_SFX_VOLUME, 3);
    audio_set_volume(&ap, options_audio_gain(&store));
    CHECK(ap.volume > 0.29f && ap.volume < 0.31f,
        "audio: saved SFX 3 opens at 0.3");
}

/* --- 4. save-context round trip --------------------------------------------- */

static void test_save_roundtrip(void)
{
    /* Run in a throwaway temp dir so nothing (no opt_fixture/, Saves/) is
     * ever left behind in the working tree. */
    char dir[] = "/tmp/opencode/lrr-opt-XXXXXX";
    if (mkdtemp(dir) == NULL || chdir(dir) != 0) {
        fprintf(stderr, "save fixture: mkdtemp/chdir failed\n");
        ++s_failures;
        return;
    }
    mkdir("Saves", 0755);

    /* Fresh game: documented defaults, and the bootstrap slot stamps them. */
    GameContext g1;
    game_context_init(&g1);
    CHECK(g1.options.brightness == 8 && g1.options.game_speed == 1,
        "save: fresh context starts at the defaults");
    CHECK(save_read_slot(&g1, 3, 1) == 0
        && g1.options.brightness == 8,
        "save: absent slot bootstraps with defaults");

    /* Change through the store, commit, reload: the values survive. */
    options_set(&g1.options, OPT_GAME_SPEED, 4);
    options_set(&g1.options, OPT_BRIGHTNESS, 2);
    options_set(&g1.options, OPT_HELP_WINDOW, 1);
    options_set(&g1.options, OPT_SFX_VOLUME, 3);
    options_set(&g1.options, OPT_MUSIC_VOLUME, 9);
    CHECK(save_write_slot(&g1, 3) == 1, "save: slot written");
    save_mark_record(&g1, 0);
    save_commit(&g1);

    GameContext g2;
    game_context_init(&g2);
    CHECK(save_read_slot(&g2, 3, 0) == 1, "save: slot reads back");
    CHECK(g2.options.game_speed == 4 && g2.options.brightness == 2
        && g2.options.help_window == 1 && g2.options.sfx_volume == 3
        && g2.options.music_volume == 9,
        "save: option values survive the save/reload round trip");

    /* An un-stamped legacy save falls back to the defaults (magic absent). */
    GameContext g3;
    game_context_init(&g3);
    CHECK(save_read_slot(&g3, 1, 1) == 0, "save: absent slot 1 -> bootstrap");
    options_set(&g3.options, OPT_BRIGHTNESS, 6);
    save_write_slot(&g3, 1);
    /* Wipe the stamp: rewrite the header's +0x20 block with zeros. */
    {
        FILE* fp = fopen("Saves/1.sav", "r+b");
        if (fp != NULL) {
            uint8_t zero[4] = { 0, 0, 0, 0 };
            fseek(fp, 0x20, SEEK_SET);
            fwrite(zero, 1, 4, fp);
            fclose(fp);
        }
    }
    GameContext g4;
    game_context_init(&g4);
    CHECK(save_read_slot(&g4, 1, 0) == 1, "save: legacy header reads back");
    CHECK(g4.options.brightness == 8 && g4.options.game_speed == 1,
        "save: un-stamped slot restores the defaults");
    if (chdir("..") != 0) {
        ++s_failures;
    }
    char rmcmd[64 + sizeof("/tmp/opencode/lrr-opt-XXXXXXXXXX")];
    snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", dir);
    system(rmcmd);
}

/* --- real-config probe (--real <wad-dir>) ----------------------------------- */

static void real_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    if (cfg == NULL) {
        fprintf(stderr, "options_test: --real probe skipped (no Lego.cfg)\n");
        return;
    }
    FrontEnd fe;
    CHECK(frontend_build(&fe, cfg) == 1, "real: menus build");

    /* Find each shipped control by its banner in the real OptionsMenu and
     * assert the menu's parsed range equals the store's shipped range. */
    int seen = 0;
    for (int m = 0; m < FE_MENU_COUNT; m++) {
        const FeMenu* menu = &fe.menus[m];
        for (int s = 0; s < menu->submenu_count; s++) {
            const FeSubmenu* sub = &menu->submenus[s];
            for (int i = 0; i < sub->item_count; i++) {
                const FeItem* it = &sub->items[i];
                int id = options_id_for_label(it->label);
                if (id < 0) {
                    continue;
                }
                if (it->kind != FE_ITEM_SLIDER && it->kind != FE_ITEM_CYCLE
                    && it->kind != FE_ITEM_REALSLIDER) {
                    continue;
                }
                int lo = 0, hi = 0;
                options_range(id, &lo, &hi);
                CHECK(it->min == lo && it->max == hi,
                    "real: shipped slider/cycle range == store range");
                ++seen;
            }
        }
    }
    CHECK(seen >= 5, "real: shipped option controls all recognized");
    fprintf(stderr,
        "options_test: --real probe: %d option controls match the store\n",
        seen);
    block_close(cfg);
}

int main(int argc, char** argv)
{
    test_store();
    test_menu_to_runtime();
    test_speed_connection();
    test_audio_restoration();
    test_save_roundtrip();
    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            real_probe(argv[a + 1]);
        }
    }
    if (s_failures == 0) {
        printf("options_test: PASS\n");
        return 0;
    }
    printf("options_test: %d failure(s)\n", s_failures);
    return 1;
}
