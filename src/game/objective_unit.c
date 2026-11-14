// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-029: the level objectives (the original's Objective.c), rebuilt in
 * original shape (ADR 0005, finding 631). Text 004574e0-00459190, .rdata
 * 0049e56c-0049e5a4, .bss 004ffbc0-004ffe4c.
 *
 * The unit loads the level's objectives (Objective_LoadLevel), the
 * briefing, completion and failure pages of the level's ObjectiveText
 * file, shows them while the level is paused, checks the objectives each
 * frame and plays the objective voice streams. The port's models of the
 * same functions stay in objective_port.cpp, unannotated: natively they
 * are the live ones and nothing calls the unit. See objective_unit.h for
 * the records. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "objective_unit_link_names.h"
#include "objective_unit.h"
#include "objective_unit_imports.h"
#include "crt/vc5names.h"

#define OBJECTIVE_DATA(level) ((Objective_Data*)(level)->f_0bc)

/* Objective.c's .bss (004ffbc0-004ffe4c), 0x28c bytes. */
typedef struct Objective_Globs {
    unsigned int flags;                     /* +0x000 */
    File* file;                             /* +0x004 */
    char filename[128];                     /* +0x008 */
    char* messages[OBJECTIVE_TYPE_COUNT];   /* +0x088 */
    unsigned char f_098[0x180];             /* +0x098 not read */
    unsigned int currentPage[OBJECTIVE_TYPE_COUNT]; /* +0x218 */
    unsigned int shownPage[OBJECTIVE_TYPE_COUNT];   /* +0x228 */
    unsigned int pageCount[OBJECTIVE_TYPE_COUNT];   /* +0x238 */
    void* textWindows[OBJECTIVE_TYPE_COUNT];        /* +0x248 */
    void* briefingTextWindow;               /* +0x258 MissionBriefingText */
    void* completedTextWindow;              /* +0x25c MissionCompletedText */
    void* failedTextWindow;                 /* +0x260 MissionFailedText */
    void* beginTextWindow;                  /* +0x264 [BEGIN] */
    unsigned int f_268;                     /* +0x268 not read */
    int beginTextShown;                     /* +0x26c */
    int achieved;                           /* +0x270 */
    unsigned int objectiveSwitch;           /* +0x274 NERPs' SetObjectiveSwitch */
    char* soundName;                        /* +0x278 */
    int soundHandle;                        /* +0x27c */
    float soundTimer;                       /* +0x280 */
    int showing;                            /* +0x284 */
    int endTeleportEnabled;                 /* +0x288 */
} Objective_Globs;

// GLOBAL: LEGORR 0x004ffbc0
static Objective_Globs objectiveGlobs;

/* Input.c's INPUT.lClicked (0076ab48), which no converted unit defines
 * yet: a common definition, as the original's. */
// GLOBAL: LEGORR 0x0076ab48
uint32_t s_window_mouse_word_48;

// FUNCTION: LEGORR 0x004574e0
void objective_load_objective_text(struct BlockRec* config,
                                   const char* gameName,
                                   const char* levelName,
                                   Lego_Level* level, const char* filename)
{
    Area2F beginArea;
    Area2F msgArea;
    int inLevelSection;
    int levelFound;
    Area2F textArea;
    const char* levelSection;
    char buff[128];
    char line[1024];
    char levelString[1024];
    char sectionName[1024];
    const char* str;
    unsigned int i;
    unsigned int len;
    unsigned int width;
    unsigned int colons;
    int type;
    int beginNext;
    int escape;
    char* s;

    textArea.x = OBJECTIVE_DATA(level)->imagePos[0] - -60.0f;
    beginArea.y = OBJECTIVE_DATA(level)->imagePos[1] - -220.0f;
    textArea.y = OBJECTIVE_DATA(level)->imagePos[1] - -60.0f;
    msgArea.x = OBJECTIVE_DATA(level)->imagePos[0] - -40.0f;
    msgArea.y = OBJECTIVE_DATA(level)->imagePos[1] - -34.0f;
    beginArea.width = 320.0f;
    beginArea.height = 20.0f;
    textArea.width = 390.0f;
    inLevelSection = 0;
    textArea.height = 180.0f;
    msgArea.width = 422.0f;
    msgArea.height = 21.0f;
    beginNext = 0;
    colons = 0;
    levelFound = 0;
    beginArea.x = textArea.x;

    sprintf(objectiveGlobs.filename, "%s", filename);

    if (objectiveGlobs.textWindows[0])
        text_buffer_destroy_target(objectiveGlobs.textWindows[0]);
    if (objectiveGlobs.briefingTextWindow)
        text_buffer_destroy_target(objectiveGlobs.briefingTextWindow);
    if (objectiveGlobs.completedTextWindow)
        text_buffer_destroy_target(objectiveGlobs.completedTextWindow);
    if (objectiveGlobs.failedTextWindow)
        text_buffer_destroy_target(objectiveGlobs.failedTextWindow);
    if (objectiveGlobs.beginTextWindow)
        text_buffer_destroy_target(objectiveGlobs.beginTextWindow);

    for (i = 0; i < OBJECTIVE_TYPE_COUNT; i++) {
        objectiveGlobs.textWindows[i] =
            text_buffer_create((uintptr_t)legoGlobs_fontBriefingHi,
                               (const uint32_t*)&textArea, 1024);
        text_output_field82c_bit2_set_clear(objectiveGlobs.textWindows[i], 0);
    }
    objectiveGlobs.beginTextWindow =
        text_buffer_create((uintptr_t)legoGlobs_fontBriefingLo,
                           (const uint32_t*)&beginArea, 1024);
    objectiveGlobs.beginTextShown = 0;

    objectiveGlobs.file = file_open(objectiveGlobs.filename, "r");
    for (i = 0; i < OBJECTIVE_TYPE_COUNT; i++) {
        if (objectiveGlobs.messages[i]) {
            free(objectiveGlobs.messages[i]);
            objectiveGlobs.messages[i] = NULL;
        }
        objectiveGlobs.pageCount[i] = 0;
        objectiveGlobs.shownPage[i] = 0;
        objectiveGlobs.currentPage[i] = 0;
    }

    if (objectiveGlobs.file) {
        file_seek(objectiveGlobs.file, 0, SEEK_SET);

        len = strlen(levelName);
        for (i = 0; i < len; i++) {
            if (colons == 2) {
                levelSection = levelName + i;
                levelFound = 1;
                break;
            }
            if (levelName[i] == ':') colons++;
        }

        while (file_gets(line, sizeof(line), objectiveGlobs.file)) {
            if (levelFound) {
                strcpy(levelString, levelSection);
                len = sprintf(sectionName,
                              // STRING: LEGORR 0x004a8104
                              "[%s]", levelString);

                if (line[0] == '[') {
                    inLevelSection = 0;
                    beginNext = 0;
                }

                if (inLevelSection) {
                    unsigned int n;

                    type = OBJECTIVE_TYPE_COUNT;
                    line[strlen(line) - 1] = '\0';
                    // STRING: LEGORR 0x004a80f8
                    if (!strnicmp(line, "Objective:", n = 10)) type = 0;
                    // STRING: LEGORR 0x004a80ec
                    else if (!strnicmp(line, "Completion:", n = 11)) type = 1;
                    // STRING: LEGORR 0x004a80e0
                    else if (!strnicmp(line, "Failure:", n = 8)) type = 2;
                    // STRING: LEGORR 0x004a80d0
                    else if (!strnicmp(line, "CrystalFailure:", n = 15)) type = 3;

                    if (type != OBJECTIVE_TYPE_COUNT) {
                        while (n < strlen(line)) {
                            if (line[n] != ' ' && line[n] != '\t') break;
                            n++;
                        }

                        escape = 0;
                        for (s = line; *s != '\0'; s++) {
                            if (escape && (*s == 'a' || *s == 'n')) {
                                s[-1] = ' ';
                                if (*s == 'a') {
                                    *s = '\a';
                                    objectiveGlobs.pageCount[type]++;
                                } else {
                                    *s = '\n';
                                }
                                escape = 0;
                            } else {
                                escape = (*s == '\\');
                            }
                        }

                        text_buffer_append_format(objectiveGlobs.textWindows[type],
                                                  "%s", line + n);
                        objectiveGlobs.messages[type] = string_duplicate(line + n);
                    }
                }

                if (beginNext) {
                    line[strlen(line) - 1] = '\0';
                    text_buffer_append_format(objectiveGlobs.beginTextWindow,
                                              "%s", line);
                    beginNext = 0;
                    objectiveGlobs.beginTextShown = 1;
                }

                if (!strnicmp(sectionName, line, len)) inLevelSection = 1;
                // STRING: LEGORR 0x004a80c8
                if (!strnicmp(line, "[BEGIN]", 7)) beginNext = 1;
            }
        }
        file_close(objectiveGlobs.file);
    }

    str = block_get_strptr(config, string_path_join(s_engine_config_root,
                                                     "Main",
                                                     // STRING: LEGORR 0x004a80b4
                                                     "MissionBriefingText",
                                                     NULL));
    if (str) {
        width = formatted_text_printf_width((const struct TextureSurface*)legoGlobs_fontBriefingLo, str);
        msgArea.x = (msgArea.width * 0.5f - (int)(width >> 1)) + msgArea.x;
        msgArea.width = (float)width;
        objectiveGlobs.briefingTextWindow =
            text_buffer_create((uintptr_t)legoGlobs_fontBriefingLo,
                               (const uint32_t*)&msgArea, 1024);
        if (objectiveGlobs.briefingTextWindow) {
            for (i = 0; i < strlen(str) + 1; i++) {
                buff[i] = (str[i] == '_') ? ' ' : (unsigned char)str[i];
            }
            text_buffer_reset_target(objectiveGlobs.briefingTextWindow);
            text_buffer_append_format(objectiveGlobs.briefingTextWindow, "%s", buff);
        }
    }

    str = block_get_strptr(config, string_path_join(s_engine_config_root,
                                                     "Main",
                                                     // STRING: LEGORR 0x004a809c
                                                     "MissionCompletedText",
                                                     NULL));
    if (str) {
        width = formatted_text_printf_width((const struct TextureSurface*)legoGlobs_fontBriefingLo, str);
        msgArea.x = (msgArea.width * 0.5f - (int)(width >> 1)) + msgArea.x;
        msgArea.width = (float)width;
        objectiveGlobs.completedTextWindow =
            text_buffer_create((uintptr_t)legoGlobs_fontBriefingLo,
                               (const uint32_t*)&msgArea, 1024);
        if (objectiveGlobs.completedTextWindow) {
            for (i = 0; i < strlen(str) + 1; i++) {
                buff[i] = (str[i] == '_') ? ' ' : (unsigned char)str[i];
            }
            text_buffer_reset_target(objectiveGlobs.completedTextWindow);
            text_buffer_append_format(objectiveGlobs.completedTextWindow, "%s", buff);
        }
    }

    str = block_get_strptr(config, string_path_join(s_engine_config_root,
                                                     "Main",
                                                     // STRING: LEGORR 0x004a8088
                                                     "MissionFailedText",
                                                     NULL));
    if (str) {
        width = formatted_text_printf_width((const struct TextureSurface*)legoGlobs_fontBriefingLo, str);
        msgArea.x = (msgArea.width * 0.5f - (int)(width >> 1)) + msgArea.x;
        msgArea.width = (float)width;
        objectiveGlobs.failedTextWindow =
            text_buffer_create((uintptr_t)legoGlobs_fontBriefingLo,
                               (const uint32_t*)&msgArea, 1024);
        if (objectiveGlobs.failedTextWindow) {
            for (i = 0; i < strlen(str) + 1; i++) {
                buff[i] = (str[i] == '_') ? ' ' : (unsigned char)str[i];
            }
            text_buffer_reset_target(objectiveGlobs.failedTextWindow);
            text_buffer_append_format(objectiveGlobs.failedTextWindow, "%s", buff);
        }
    }

    if (!objectiveGlobs.beginTextShown) {
        textArea.height += beginArea.height;
        for (i = 0; i < 3; i++) {
            float_pair_at_offsetc_set_from_uints(objectiveGlobs.textWindows[i],
                                                 (unsigned int)textArea.width,
                                                 (unsigned int)textArea.height);
        }
    }
}

// FUNCTION: LEGORR 0x00457d40
void objective_load_level(struct BlockRec* config, const char* gameName,
                          const char* levelName, Lego_Level* level,
                          unsigned int screenWidth, unsigned int screenHeight)
{
    int objType;
    unsigned int objID;
    int blockPos[2];
    char* argv[60];
    char buff[1024];
    int hitTimeFail = 0;
    const char* str;
    char* s;
    int argc;
    unsigned int r, g, b;
    float timer;

    memset(OBJECTIVE_DATA(level), 0, sizeof(Objective_Data));
    objectiveGlobs.flags = 0;
    objectiveGlobs.objectiveSwitch = 1;

    if (block_get_bool_path(config, string_path_join(gameName, levelName,
            // STRING: LEGORR 0x004a8234
            "DontShowObjectiveAdvisor", NULL)) != 1)
        objectiveGlobs.flags |= OBJECTIVE_FLAG_SHOWBRIEFINGADVISOR;
    if (block_get_bool_path(config, string_path_join(gameName, levelName,
            // STRING: LEGORR 0x004a8214
            "DontShowObjectiveAcheiveAdvisor", NULL)) != 1)
        objectiveGlobs.flags |= OBJECTIVE_FLAG_SHOWACHIEVEADVISOR;
    if (block_get_bool_path(config, string_path_join(gameName, levelName,
            // STRING: LEGORR 0x004a81f4
            "DontShowObjectiveFailedAdvisor", NULL)) != 1)
        objectiveGlobs.flags |= OBJECTIVE_FLAG_SHOWFAILEDADVISOR;

    sprintf(buff,
            // STRING: LEGORR 0x004a81e0
            "ObjectiveImage%ix%i", screenWidth, screenHeight);
    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         buff, NULL)))) {
        s = string_duplicate(str);
        argc = sound_variant_split(s, argv, ",");
        if (OBJECTIVE_DATA(level)->image) image_surface_release(OBJECTIVE_DATA(level)->image);
        OBJECTIVE_DATA(level)->image = image_load_bmp_scaled(argv[0], 0, 0);
        if (OBJECTIVE_DATA(level)->image) resource_record_add_ref(OBJECTIVE_DATA(level)->image);
        OBJECTIVE_DATA(level)->imagePos[0] = (float)atoi(argv[1]);
        OBJECTIVE_DATA(level)->imagePos[1] = (float)atoi(argv[2]);
        if (argc == 6 && OBJECTIVE_DATA(level)->image) {
            r = atoi(argv[3]);
            g = atoi(argv[4]);
            b = atoi(argv[5]);
            image_surface_blit_transformed((struct TextureSurface*)OBJECTIVE_DATA(level)->image,
                                           (float)(r / 255), (float)(g / 255),
                                           (float)(b / 255), (float)(r / 255),
                                           (float)(g / 255), (float)(b / 255));
        }
        free(s);
    }

    sprintf(buff,
            // STRING: LEGORR 0x004a81c4
            "ObjectiveAcheivedImage%ix%i", screenWidth, screenHeight);
    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         buff, NULL)))) {
        s = string_duplicate(str);
        argc = sound_variant_split(s, argv, ",");
        if (OBJECTIVE_DATA(level)->achievedImage) image_surface_release(OBJECTIVE_DATA(level)->achievedImage);
        OBJECTIVE_DATA(level)->achievedImage = image_load_bmp_scaled(argv[0], 0, 0);
        if (OBJECTIVE_DATA(level)->achievedImage) resource_record_add_ref(OBJECTIVE_DATA(level)->achievedImage);
        OBJECTIVE_DATA(level)->achievedImagePos[0] = (float)atoi(argv[1]);
        OBJECTIVE_DATA(level)->achievedImagePos[1] = (float)atoi(argv[2]);
        if (argc == 6 && OBJECTIVE_DATA(level)->achievedImage) {
            r = atoi(argv[3]);
            g = atoi(argv[4]);
            b = atoi(argv[5]);
            /* The original sets the briefing image's transparency here. */
            image_surface_blit_transformed((struct TextureSurface*)OBJECTIVE_DATA(level)->image,
                                           (float)(r / 255), (float)(g / 255),
                                           (float)(b / 255), (float)(r / 255),
                                           (float)(g / 255), (float)(b / 255));
        }
        free(s);
    }

    if ((s = block_get_str_copy(config, string_path_join(gameName, levelName,
                                                         // STRING: LEGORR 0x004a81ac
                                                         "ObjectiveAcheivedAVI",
                                                         NULL)))) {
        argc = sound_variant_split(s, argv, ",");
        OBJECTIVE_DATA(level)->achievedVideoName = string_duplicate(argv[0]);
        if (argc == 3) {
            OBJECTIVE_DATA(level)->achievedVideoPos[0] = (float)atoi(argv[1]);
            OBJECTIVE_DATA(level)->achievedVideoPos[1] = (float)atoi(argv[2]);
            free(s);
        } else {
            OBJECTIVE_DATA(level)->noAchievedVideoPos = 1;
            free(s);
        }
    }

    sprintf(buff,
            // STRING: LEGORR 0x004a8190
            "ObjectiveFailedImage%ix%i", screenWidth, screenHeight);
    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         buff, NULL)))) {
        s = string_duplicate(str);
        sound_variant_split(s, argv, ",");
        if (OBJECTIVE_DATA(level)->failedImage) image_surface_release(OBJECTIVE_DATA(level)->failedImage);
        OBJECTIVE_DATA(level)->failedImage = image_load_bmp_scaled(argv[0], 0, 0);
        if (OBJECTIVE_DATA(level)->failedImage) resource_record_add_ref(OBJECTIVE_DATA(level)->failedImage);
        OBJECTIVE_DATA(level)->failedImagePos[0] = (float)atoi(argv[1]);
        OBJECTIVE_DATA(level)->failedImagePos[1] = (float)atoi(argv[2]);
        free(s);
    }

    objective_set_cry_ore_objectives(level, atoi(block_get_strptr(config, string_path_join(gameName, levelName,
                        // STRING: LEGORR 0x004a817c
                        "CrystalObjective", NULL))
                    ? block_get_strptr(config, string_path_join(gameName, levelName,
                          "CrystalObjective", NULL))
                    : s_config_angle_empty),
               atoi(block_get_strptr(config, string_path_join(gameName, levelName,
                   // STRING: LEGORR 0x004a816c
                   "OreObjective", NULL))
               ? block_get_strptr(config, string_path_join(gameName, levelName,
                     "OreObjective", NULL))
               : s_config_angle_empty));

    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         // STRING: LEGORR 0x004a815c
                                                         "TimerObjective",
                                                         NULL)))) {
        s = string_duplicate(str);
        sound_variant_split(s, argv, ":");
        timer = (float)atoi(argv[0]) * 25.0f;
        // STRING: LEGORR 0x004a8144
        if (!_strcmpi(argv[1], "HitTimeFailObjective")) hitTimeFail = 1;
        objective_set_timer_objective(level, timer, hitTimeFail);
        free(s);
    }

    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         // STRING: LEGORR 0x004a812c
                                                         "ConstructionObjective",
                                                         NULL))) &&
        lego_entity_type_pair_lookup(str, &objType, &objID, NULL)) {
        objective_set_construction_objective(level, objType, (int)objID);
    }

    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         // STRING: LEGORR 0x004a811c
                                                         "BlockObjective",
                                                         NULL)))) {
        s = string_duplicate(str);
        sound_variant_split(s, argv, ",");
        blockPos[0] = atoi(argv[0]);
        blockPos[1] = atoi(argv[1]);
        objective_set_block_objective(level, blockPos);
        free(s);
    }

    if ((str = block_get_strptr(config, string_path_join(gameName, levelName,
                                                         // STRING: LEGORR 0x004a810c
                                                         "ObjectiveText",
                                                         NULL)))) {
        objective_load_objective_text(config, gameName, levelName, level, str);
    }

    if (objectiveGlobs.flags == 0)
        objectiveGlobs.flags = OBJECTIVE_FLAG_SHOWBRIEFINGADVISOR;
}

// FUNCTION: LEGORR 0x00458580
void objective_set_cry_ore_objectives(Lego_Level* level, unsigned int crystals,
                                      unsigned int ore)
{
    if (crystals) {
        objectiveGlobs.flags |= OBJECTIVE_FLAG_CRYSTAL;
        OBJECTIVE_DATA(level)->crystals = crystals;
    }
    if (ore) {
        objectiveGlobs.flags |= OBJECTIVE_FLAG_ORE;
        OBJECTIVE_DATA(level)->ore = ore;
    }
}

// FUNCTION: LEGORR 0x004585c0
void objective_set_block_objective(Lego_Level* level, const int* blockPos)
{
    objectiveGlobs.flags |= OBJECTIVE_FLAG_BLOCK;
    OBJECTIVE_DATA(level)->block[0] = blockPos[0];
    OBJECTIVE_DATA(level)->block[1] = blockPos[1];
}

// FUNCTION: LEGORR 0x004585f0
void objective_set_timer_objective(Lego_Level* level, float timer,
                                   int hitTimeFail)
{
    objectiveGlobs.flags |= OBJECTIVE_FLAG_TIMER;
    OBJECTIVE_DATA(level)->timer = timer;
    if (hitTimeFail) objectiveGlobs.flags |= OBJECTIVE_FLAG_HITTIMEFAIL;
}

// FUNCTION: LEGORR 0x00458620
void objective_set_construction_objective(Lego_Level* level, int objType,
                                          int objID)
{
    objectiveGlobs.flags |= OBJECTIVE_FLAG_CONSTRUCTION;
    OBJECTIVE_DATA(level)->constructionType = objType;
    OBJECTIVE_DATA(level)->constructionID = objID;
}

// FUNCTION: LEGORR 0x00458650
int objective_has_achieved(void)
{
    return objectiveGlobs.achieved;
}

// FUNCTION: LEGORR 0x00458660
void objective_set_end_teleport_enabled(int on)
{
    objectiveGlobs.endTeleportEnabled = on;
}

// FUNCTION: LEGORR 0x00458670
void objective_set_status(int status)
{
    Lego_Level* level = (Lego_Level*)level_current_owner_get();
    const char* statusName = NULL;
    int teleport = 1;
    int useLevelName = 1;
    char buff[256];
    unsigned int teleportFlags;

    if (!nerpfunc_get_tutorial_flags(NULL)) s_panel_highlight_flags |= 0x1000;

    memset(objectiveGlobs.currentPage, 0, sizeof(objectiveGlobs.currentPage));

    if (objectiveGlobs.flags & (OBJECTIVE_FLAG_BRIEFING |
                                OBJECTIVE_FLAG_COMPLETED |
                                OBJECTIVE_FLAG_FAILED)) return;
    if (objectiveGlobs.flags & OBJECTIVE_FLAG_CRYSTAL) return;

    while (g_renderer_word_c || s_window_mouse_word_40 ||
           s_window_mouse_word_48 || s_window_mouse_word_4c) {
        s_window_mouse_word_4c = 0;
        main_loop_frame_pump(1);
    }

    if (status == 0) {
        statusName = s_config_angle_empty;
        objectiveGlobs.flags |= OBJECTIVE_FLAG_BRIEFING;
        teleport = 0;
        level->status = 0;
    } else {
        if (status == 1) {
            objectiveGlobs.achieved = 1;
            objectiveGlobs.flags |= OBJECTIVE_FLAG_COMPLETED;
            level->status = 1;
            // STRING: LEGORR 0x004a8284
            statusName = "Acheived";
            objective_image_get_screenshot_adapter(&rewardGlobs.level.saveCapture,
                                                   frontGlobs_saveImageBigWidth,
                                                   frontGlobs_saveImageBigHeight);
            rewardGlobs.level.saveHasCapture = 1;
        } else if (status == 2) {
            objectiveGlobs.achieved = 0;
            objectiveGlobs.flags |= OBJECTIVE_FLAG_FAILED;
            // STRING: LEGORR 0x004a827c
            statusName = "Failed";
            level->status = 2;
        } else if (status == 3) {
            objectiveGlobs.achieved = 0;
            objectiveGlobs.flags |= OBJECTIVE_FLAG_FAILED | OBJECTIVE_FLAG_CRYSTAL;
            // STRING: LEGORR 0x004a826c
            statusName = "FailedCrystals";
            level->status = 2;
            useLevelName = 0;
        } else {
            goto status_read;
        }
        lego_global_subsystem_state_reset();
        reward_level_start();
    }

status_read:
    objectiveGlobs.flags |= OBJECTIVE_FLAG_STATUSREAD;

    if (teleport) {
        teleportFlags = objectiveGlobs.endTeleportEnabled ? 2 : 4;
        teleporter_start(1, 2, teleportFlags);
        teleporter_start(2, 2, teleportFlags);
        teleporter_start(8, 2, teleportFlags);
        teleporter_start(0x400, 2, teleportFlags);
        level_state_stored_value_set(1);
        legoobject_scatter_spawn_gate_set(1);
    }

    if (statusName && (legoGlobs_flags1 & 0x10)) {
        sprintf(buff,
                // STRING: LEGORR 0x004a8250
                "Stream_Objective%s%s%s", statusName,
                // STRING: LEGORR 0x004a8268
                useLevelName ? "_" : s_config_angle_empty,
                useLevelName ? LEGO_LEVEL_PTR(char, level->name) : s_config_angle_empty);
        if (objectiveGlobs.soundName) free(objectiveGlobs.soundName);
        objectiveGlobs.soundName = string_duplicate(buff);
        objectiveGlobs.soundHandle = -1;
        objectiveGlobs.showing = 0;
    } else {
        objectiveGlobs.soundName = NULL;
    }
}

// FUNCTION: LEGORR 0x004588e0
void objective_stop_showing_objective(void)
{
    if (objectiveGlobs.flags & OBJECTIVE_FLAG_BRIEFING) {
        nerpfunc_set_objective_switch(&objectiveGlobs.objectiveSwitch);
        objectiveGlobs.objectiveSwitch = 0;
        objectiveGlobs.flags &= ~OBJECTIVE_FLAG_BRIEFING;
        if (s_panel_highlight_flags & 0x800) {
            lego_cd_audio_watchdog_mode_update(1);
            s_panel_highlight_flags &= ~0x800;
        }
    } else if (objectiveGlobs.flags & OBJECTIVE_FLAG_COMPLETED) {
        objectiveGlobs.flags &= ~OBJECTIVE_FLAG_COMPLETED;
    } else if (objectiveGlobs.flags & OBJECTIVE_FLAG_FAILED) {
        objectiveGlobs.flags &= ~(OBJECTIVE_FLAG_FAILED | OBJECTIVE_FLAG_CRYSTAL);
    }

    advisor_clear_active_state();
    if (objectiveGlobs.soundHandle != -1) {
        sound_stream_stop_and_release(0);
        objectiveGlobs.soundHandle = -1;
    }
    lego_transition_control(0, 0);
    sfx_set_queue_mode_and_flush(0);
    if (!nerpfunc_get_tutorial_flags(NULL)) s_panel_highlight_flags &= ~0x1000;
}

// FUNCTION: LEGORR 0x004589a0
int objective_is_ended(void)
{
    if (objectiveGlobs.flags & OBJECTIVE_FLAG_BRIEFING ||
        objectiveGlobs.flags & OBJECTIVE_FLAG_COMPLETED ||
        objectiveGlobs.flags & OBJECTIVE_FLAG_FAILED) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x004589c0
int objective_handle_keys(int nextKeyPressed, int leftButtonReleased,
                          int* outKeyPressed)
{
    unsigned int type = OBJECTIVE_TYPE_COUNT;
    int inNextButton;

    if (objectiveGlobs.flags & OBJECTIVE_FLAG_BRIEFING) type = 0;
    else if (objectiveGlobs.flags & OBJECTIVE_FLAG_COMPLETED) type = 1;
    else if (objectiveGlobs.flags & OBJECTIVE_FLAG_FAILED)
        type = (objectiveGlobs.flags & OBJECTIVE_FLAG_CRYSTAL) ? 3 : 2;

    *outKeyPressed = 0;

    if ((nextKeyPressed || leftButtonReleased) &&
        objectiveGlobs.currentPage[type] == objectiveGlobs.pageCount[type]) {
        if ((int)s_renderer_word_a < 130 ||
            (int)s_renderer_word_a >= (int)legoGlobs_BackButtonImage->width + 130 ||
            (int)s_renderer_word_b < 315 ||
            (int)s_renderer_word_b >= (int)legoGlobs_BackButtonImage->height + 315) {
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_BRIEFING) {
                objective_stop_showing_objective();
                objective_score_text_origin_set(NULL);
                lego_objective_task_text_origin_set(NULL);
                lego_transition_control(0, 0);
                input_cursor_client_position_set(s_display_w / 2, s_display_h / 2);
                if (bubble_get_object_uis_always_visible())
                    bubble_toggle_object_uis_always_visible();
            } else if (objectiveGlobs.flags & (OBJECTIVE_FLAG_COMPLETED |
                                               OBJECTIVE_FLAG_FAILED)) {
                if (teleporter_service_all(0x40b)) {
                    objective_stop_showing_objective();
                    *outKeyPressed = !lego_keep_running();
                }
            }
        }
    }

    if (type != OBJECTIVE_TYPE_COUNT) {
        if (objectiveGlobs.currentPage[type] < objectiveGlobs.pageCount[type]) {
            if (!((int)s_renderer_word_a < 470 ||
                (int)s_renderer_word_a >= (int)legoGlobs_NextButtonImage->width + 470 ||
                (int)s_renderer_word_b < 315 ||
                (int)s_renderer_word_b >= (int)legoGlobs_NextButtonImage->height + 315)) {
                tooltip_activate(0x21);
                if (leftButtonReleased) goto next_page_sound;
            }
            if (nextKeyPressed) {
                if (leftButtonReleased) {
next_page_sound:
                    media_action_feedback_sound_dispatch(0);
                }
                objectiveGlobs.currentPage[type]++;
            }
        }

        if (objectiveGlobs.currentPage[type] > 0 &&
            (int)s_renderer_word_a > 129 &&
            (int)s_renderer_word_a < (int)legoGlobs_BackButtonImage->width + 130 &&
            (int)s_renderer_word_b > 314 &&
            (int)s_renderer_word_b < (int)legoGlobs_BackButtonImage->height + 315) {
            tooltip_activate(0x22);
            if (leftButtonReleased) {
                media_action_feedback_sound_dispatch(0);
                objectiveGlobs.currentPage[type]--;
            }
        }
    }

    return objective_is_ended();
}

// FUNCTION: LEGORR 0x00458be0
void objective_update(void* textWnd, Lego_Level* level, float elapsedGame,
                      float elapsedInterface)
{
    int setQueueMode;
    int type;
    int completed;
    int playMusic = 0;
    int showTextWindows = 0;
    int showing = 0;
    void* textWindow;

    setQueueMode = 0;

    if ((unsigned int)s_application_mode > 3)
        objective_stop_showing_objective();

    if (objectiveGlobs.flags & OBJECTIVE_FLAG_BRIEFING) {
        showing = 1;
        lego_transition_control(0, 1);
        type = 0;
        showTextWindows = 1;
        if (objectiveGlobs.flags & OBJECTIVE_FLAG_STATUSREAD) {
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_SHOWBRIEFINGADVISOR)
                advisor_request_admit(0, 1);
            objectiveGlobs.flags &= ~OBJECTIVE_FLAG_STATUSREAD;
        }
        setQueueMode = 1;
        text_display_message(0x16, 0, 0);
    } else if (objectiveGlobs.flags & OBJECTIVE_FLAG_COMPLETED) {
        if (teleporter_service_all(0x40b)) {
            playMusic = 1;
            lego_transition_control(0, 1);
            objective_osf_store_adapter();
            if (!OBJECTIVE_DATA(level)->achievedVideoPlayed) {
                lego_play_movie(OBJECTIVE_DATA(level)->achievedVideoName,
                                OBJECTIVE_DATA(level)->noAchievedVideoPos
                                    ? NULL
                                    : OBJECTIVE_DATA(level)->achievedVideoPos);
                OBJECTIVE_DATA(level)->achievedVideoPlayed = 1;
            }
            type = 1;
            showTextWindows = 1;
            text_display_message(0x16, 0, 0);
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_STATUSREAD) {
                if (objectiveGlobs.flags & OBJECTIVE_FLAG_SHOWACHIEVEADVISOR)
                    advisor_request_admit(0, 1);
                objectiveGlobs.flags &= ~OBJECTIVE_FLAG_STATUSREAD;
            }
            showing = 1;
        }
    } else if (objectiveGlobs.flags & OBJECTIVE_FLAG_FAILED) {
        if (teleporter_service_all(0x40b)) {
            type = 2;
            objective_osf_restore_adapter();
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_CRYSTAL) type = 3;
            lego_transition_control(0, 1);
            showTextWindows = 1;
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_STATUSREAD) {
                if (objectiveGlobs.flags & OBJECTIVE_FLAG_SHOWFAILEDADVISOR)
                    advisor_request_admit(0, 1);
                objectiveGlobs.flags &= ~OBJECTIVE_FLAG_STATUSREAD;
            }
            text_display_message(0x16, 0, 0);
            showing = 1;
            playMusic = 1;
        }
    } else {
        if (objective_check_completed(level, &completed, elapsedGame)) {
            if (completed) objective_set_status(1);
            else objective_set_status(2);
        } else {
            reward_score_add(elapsedGame);
        }
    }

    if (objectiveGlobs.soundHandle != -1) {
        if ((objectiveGlobs.soundTimer -= elapsedInterface) <= 0.0f) {
            advisor_clear_active_state();
            objectiveGlobs.soundHandle = -1;
        }
    }

    if (playMusic && !(s_panel_highlight_flags & 0x800) &&
        (legoGlobs_flags1 & 0x8)) {
        s_panel_highlight_flags |= 0x800;
        lego_cd_audio_watchdog_mode_update(0);
    }

    if (objectiveGlobs.showing && objectiveGlobs.soundName) {
        int sfxID;

        if (sfx_get_type(objectiveGlobs.soundName, &sfxID)) {
            sfx_is_queue_mode();
            sfx_set_queue_mode(0, 0);
            objectiveGlobs.soundHandle = sfx_random_play_sound_normal(sfxID, 0);
            objectiveGlobs.soundTimer =
                (sfx_random_get_sample_play_time(sfxID) - 1.5f) * 25.0f;
            sfx_set_queue_mode_and_flush(1);
        }
        free(objectiveGlobs.soundName);
        objectiveGlobs.soundName = NULL;
    }
    objectiveGlobs.showing = showing;

    if (setQueueMode) sfx_set_queue_mode_and_flush(1);

    if (showTextWindows) {
        float pos[2];

        if (objectiveGlobs.currentPage[type] < objectiveGlobs.pageCount[type]) {
            pos[0] = 470.0f;
            pos[1] = 315.0f;
            objective_score_text_origin_set(pos);
        } else {
            objective_score_text_origin_set(NULL);
        }
        if (objectiveGlobs.currentPage[type]) {
            pos[0] = 130.0f;
            pos[1] = 315.0f;
            lego_objective_task_text_origin_set(pos);
        } else {
            lego_objective_task_text_origin_set(NULL);
        }

        if (objectiveGlobs.currentPage[type] != objectiveGlobs.shownPage[type]) {
            text_buffer_reset_target(objectiveGlobs.textWindows[type]);
            formatted_text_append_to_ring_buffer(objectiveGlobs.textWindows[type],
                                                 objectiveGlobs.currentPage[type],
                                                 "%s",
                                                 objectiveGlobs.messages[type]);
            objectiveGlobs.shownPage[type] = objectiveGlobs.currentPage[type];
        }

        if (OBJECTIVE_DATA(level)->image)
            image_display_scaled(OBJECTIVE_DATA(level)->image, NULL,
                                 (const Point2F*)OBJECTIVE_DATA(level)->imagePos,
                                 NULL);

        switch (level->status) {
        case 1: textWindow = objectiveGlobs.completedTextWindow; break;
        case 2: textWindow = objectiveGlobs.failedTextWindow; break;
        default: textWindow = objectiveGlobs.briefingTextWindow; break;
        }
        if (textWindow) text_buffer_draw_wrapped(textWindow, 0, elapsedGame, NULL);

        if (objectiveGlobs.textWindows[type])
            text_buffer_draw_wrapped(objectiveGlobs.textWindows[type], 0,
                                     elapsedGame, NULL);
        if (objectiveGlobs.beginTextWindow)
            text_buffer_draw_wrapped(objectiveGlobs.beginTextWindow, 0,
                                     elapsedGame, NULL);
    }
}

// FUNCTION: LEGORR 0x00459050
int objective_check_completed(Lego_Level* level, int* outCompleted,
                              float elapsed)
{
    if (objectiveGlobs.flags & OBJECTIVE_FLAG_SHOWBRIEFINGADVISOR) return 0;

    *outCompleted = 1;

    if (objectiveGlobs.flags & OBJECTIVE_FLAG_TIMER) {
        if ((OBJECTIVE_DATA(level)->timer -= elapsed) <= 0.0f) {
            if (objectiveGlobs.flags & OBJECTIVE_FLAG_HITTIMEFAIL)
                *outCompleted = 0;
            return 1;
        }
    }

    if ((objectiveGlobs.flags & OBJECTIVE_FLAG_CRYSTAL) &&
        (unsigned int)level->f_08c < OBJECTIVE_DATA(level)->crystals) return 0;
    if ((objectiveGlobs.flags & OBJECTIVE_FLAG_ORE) &&
        (unsigned int)level->crystals < OBJECTIVE_DATA(level)->ore) return 0;

    return legoobject_run_through_lists_skip_upgrade_parts(
               objective_callback_check_completed_object,
               OBJECTIVE_DATA(level)) != 0;
}

// FUNCTION: LEGORR 0x00459100
int objective_callback_check_completed_object(struct Lego_Object* liveObj,
                                              void* data)
{
    Objective_Data* objective = (Objective_Data*)data;
    int blockPos[2];

    if (objectiveGlobs.flags & OBJECTIVE_FLAG_BLOCK) {
        if (LEGO_FIELD(liveObj, int, 0x00) == 2 ||
            LEGO_FIELD(liveObj, int, 0x00) == 1) {
            legoobject_position_to_terrain_cell(liveObj, &blockPos[0],
                                                &blockPos[1]);
            if (blockPos[0] == objective->block[0] &&
                blockPos[1] == objective->block[1]) return 1;
        }
    }

    if (objectiveGlobs.flags & OBJECTIVE_FLAG_CONSTRUCTION) {
        if (objective->constructionType == LEGO_FIELD(liveObj, int, 0x00) &&
            objective->constructionID == LEGO_FIELD(liveObj, int, 0x04)) return 1;
    }

    return 0;
}
