// SPDX-License-Identifier: LGPL-3.0-or-later
// Mission task table + reward config (finding 09).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/game.hpp"
#include "game/level.hpp"
#include "game/mission.hpp"
#include "game/objective.hpp"
#include "gods98/block.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static const char* FIX = "mission_fixture";

int main(void)
{
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/Lego.cfg", FIX);
        FILE* fp = fopen(path, "w");
        if (fp != NULL) {
            fputs(
                "Lego {\n"
                "  Level01 {\n"
                "    Reward {\n"
                "      Enable TRUE\n"
                "      Modifier 2\n"
                "      Timer 60\n"
                "      Display TRUE\n"
                "      CentreText TRUE\n"
                "      ScrollSpeed 80\n"
                "      VertSpacing 20\n"
                "      TitleFont TITLE\n"
                "      BackFont BACK\n"
                "      CompleteText COMPLETE\n"
                "      FailedText FAILED\n"
                "      QuitText QUIT\n"
                "      TextPos 100|200\n"
                "      SaveButton save_a.bmp|10|10\n"
                "      AdvanceButton adv_b.bmp|300|10\n"
                "      Importance { Crystals 60  Ore 10  Timer 20 }\n"
                "      Quota { Crystals 5  Ore 3  Timer 900 }\n"
                "      Images { Crystals img_c.bmp|1|2 }\n"
                "      BoxImages { Crystals box_c.bmp|3|4 }\n"
                "      Flics { Crystals flic_c.flc|0|0|20|20 }\n"
                "      Fonts { Crystals FNTX }\n"
                "      Sounds { Crystals SNDX }\n"
                "    }\n"
                "  }\n"
                "  Level02 { Reward { Quota { Crystals 100 } } }\n"
                "}\n",
                fp);
            fclose(fp);
        }
    }
    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }

    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");

    GameContext ctx;
    game_context_init(&ctx);
    ctx.cfg = cfg;
    LevelState* lvl = (LevelState*)calloc(1, sizeof(LevelState));
    lvl->ptr_name = (void*)"Level01";
    ctx.level = lvl;

    mission_task_init(&ctx, cfg, "Level01");

    /* The ten named slots, in order. */
    CHECK(strcmp(task_name(&ctx.tasks[0]), "Crystals") == 0, "task 0 name");
    CHECK(strcmp(task_name(&ctx.tasks[1]), "Ore") == 0, "task 1 name");
    CHECK(strcmp(task_name(&ctx.tasks[8]), "Timer") == 0, "task 8 name");
    CHECK(strcmp(task_name(&ctx.tasks[9]), "Score") == 0, "task 9 name");

    /* Importance / quota refreshers. */
    CHECK(task_importance(&ctx.tasks[0]) == 60.0f, "Crystals importance");
    CHECK(task_importance(&ctx.tasks[1]) == 10.0f, "Ore importance");
    CHECK(task_importance(&ctx.tasks[8]) == 20.0f, "Timer importance");
    CHECK(task_quota(&ctx.tasks[0]) == 5, "Crystals quota");
    CHECK(task_quota(&ctx.tasks[1]) == 3, "Ore quota");
    CHECK(task_quota(&ctx.tasks[8]) == 900, "Timer quota");

    /* Crystal quota budget (<= 26), checked on an isolated context so the
     * main context's task quotas stay the Level01 values. */
    CHECK(ctx.crystal_quota == 5, "crystal quota from config");
    {
        GameContext c2;
        game_context_init(&c2);
        c2.cfg = cfg;
        LevelState* l2 = (LevelState*)calloc(1, sizeof(LevelState));
        l2->ptr_name = (void*)"Level02";
        c2.level = l2;
        mission_task_refresh(&c2);
        CHECK(c2.crystal_quota == 26, "crystal quota clamped to 26");
        free(l2);
    }

    /* Field loaders: images/box/flic/font/sound capture + flags. */
    CHECK((task_flags(&ctx.tasks[0]) & TASK_FLAG_IMG) != 0, "Crystals image flag");
    CHECK((task_flags(&ctx.tasks[0]) & TASK_FLAG_BOXIMG) != 0, "Crystals box flag");
    CHECK((task_flags(&ctx.tasks[0]) & TASK_FLAG_FLIC) != 0, "Crystals flic flag");
    CHECK(memcmp(ctx.tasks[0].data + TASK_OFF_FONT, "FNTX", 4) == 0, "font token");
    CHECK(memcmp(ctx.tasks[0].data + TASK_OFF_SOUND, "SNDX", 4) == 0, "sound token");

    /* Reward screen config. */
    mission_reward_init(&ctx, cfg, "Level01");
    {
        RewardScreen* rs = &ctx.reward_screen;
        CHECK(rs->display == 1, "Reward Display TRUE -> 1");
        CHECK(rs->centre_text == 1, "CentreText");
        CHECK(rs->scroll_speed == 80.0f, "ScrollSpeed");
        CHECK(rs->vert_spacing == 20.0f, "VertSpacing");
        CHECK(strcmp(rs->font_title, "TITLE") == 0, "TitleFont");
        CHECK(strcmp(rs->font_back, "BACK") == 0, "BackFont");
        CHECK(strcmp(rs->complete_text, "COMPLETE") == 0, "CompleteText");
        CHECK(rs->text_pos[0] == 100 && rs->text_pos[1] == 200, "TextPos");
        CHECK(rs->save_x == 10.0f && rs->save_y == 10.0f, "SaveButton zone");
        CHECK(rs->advance_x == 300.0f && rs->advance_y == 10.0f, "AdvanceButton zone");
    }

    /* Live bridge: counters feed the objective live slots + task values. */
    mission_set_crystals(&ctx, 5);
    mission_set_ore(&ctx, 3);
    CHECK(mission_update(&ctx) != 0, "tasks complete at quota");
    CHECK(level_int(lvl, OBJ_OFF_CRYSTAL_LIVE) == 5, "objective crystal live slot");
    CHECK(level_int(lvl, OBJ_OFF_ORE_LIVE) == 3, "objective ore live slot");
    CHECK((task_flags(&ctx.tasks[0]) & TASK_FLAG_COMPLETED) != 0,
          "Crystals task completed");
    CHECK((task_flags(&ctx.tasks[1]) & TASK_FLAG_COMPLETED) != 0, "Ore task completed");
    /* Idempotent: a second update reports nothing new. */
    CHECK(mission_update(&ctx) == 0, "no new completions on second update");

    free(lvl);
    block_close(cfg);
    if (s_failures != 0) {
        fprintf(stderr, "mission_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("mission_test: OK\n");
    return 0;
}
