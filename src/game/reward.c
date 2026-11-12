// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-037: the level reward screen (the original's Reward.c), rebuilt in
 * original shape (ADR 0005, finding 560). Text 0045f030-00462da0. See
 * reward.h for the state and the port's split. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "reward.h"
#include "crt/crt.h"
#include "crt/crt_mouse.h"
#include "crt/vc5names.h"
#if !defined(_MSC_VER)
#include "gods98/winmm.h"
#endif
#include "gods98/utils.h"
#include "gods98/sound.h"
#include "gods98/sound_api.h"
#include "gods98/sound3d.h"
#include "gods98/image_adapters.h"
#include "gods98/flic.h"
#include "gods98/flic_api.h"
#include "gods98/textwindow.h"
#include "gods98/font_api.h"
#include "gods98/frameloop.h"
#include "gods98/input_keys.h"
#include "gods98/media.h"
#include "gods98/record_api.h"
#include "gods98/view_api.h"
#include "advisor.h"
#include "effects.h"
#include "entity_api.h"
#include "frontend.h"
#include "frontend_api.h"
#include "lego.h"
#include "lego_api.h"
#include "level.h"
#include "mission.h"
#include "options.h"
#include "savegame.h"
#include "terrain_grid.h"
#include "upgrades.h"
#include "video_graph.h"

/* Flic.c's current frame (+0x624). The port's Flic has another layout. */
#define REWARD_FLIC_FRAME(flic) (*(int*)((char*)(flic) + 0x624))

/* Input.c's mouse position is signed; the port's words are unsigned. */
#define REWARD_MOUSE_X ((int)s_renderer_word_a)
#define REWARD_MOUSE_Y ((int)s_renderer_word_b)

/* FrontEnd.c's level index of the level being scored. */
#define REWARD_LEVEL_INDEX(name) \
    front_level_set_index_of((const struct SavListWindow*)active_object_list_word(), (name))

/* The state, in the common region. */
// GLOBAL: LEGORR 0x00552980
RewardGlobs rewardGlobs;

// FUNCTION: LEGORR 0x0045f030
int reward_initialise(void)
{
    const char* names[REWARD_ITEM_COUNT];
    unsigned int i;

    names[REWARD_ITEM_CRYSTALS] = "Crystals";
    names[REWARD_ITEM_ORE] = "Ore";
    // STRING: LEGORR 0x004a9364
    names[REWARD_ITEM_DIGGABLE] = "Diggable";
    // STRING: LEGORR 0x004a9354
    names[REWARD_ITEM_CONSTRUCTIONS] = "Constructions";
    // STRING: LEGORR 0x004a934c
    names[REWARD_ITEM_CAVERNS] = "Caverns";
    // STRING: LEGORR 0x004a9344
    names[REWARD_ITEM_FIGURES] = "Figures";
    // STRING: LEGORR 0x004a9334
    names[REWARD_ITEM_ROCKMONSTERS] = "RockMonsters";
    // STRING: LEGORR 0x004a932c
    names[REWARD_ITEM_OXYGEN] = "Oxygen";
    // STRING: LEGORR 0x004a9324
    names[REWARD_ITEM_TIMER] = "Timer";
    // STRING: LEGORR 0x004a931c
    names[REWARD_ITEM_SCORE] = "Score";

    rewardGlobs.itemPart = (RewardLevel*)malloc(sizeof(RewardLevel));
    memset(rewardGlobs.itemPart, 0, sizeof(RewardLevel));
    rewardGlobs.itemPart->items = rewardGlobs.itemPart->itemsArray;
    rewardGlobs.itemPart->itemCount = REWARD_ITEM_COUNT;

    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        sprintf(rewardGlobs.itemPart->items[i].name, "%s", names[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_image(&rewardGlobs.itemPart->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_text(&rewardGlobs.itemPart->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_flic(&rewardGlobs.itemPart->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_font(&rewardGlobs.itemPart->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_sound(&rewardGlobs.itemPart->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_box_image(&rewardGlobs.itemPart->items[i]);

    return 1;
}

// FUNCTION: LEGORR 0x0045f1f0
void reward_load_item_sound(RewardLevelItem* item)
{
    item->soundName = block_get_str_copy(rewardGlobs.config,
        string_path_join(rewardGlobs.gameName, "Reward",
                         // STRING: LEGORR 0x004a9370
                         "Sounds",
                         item->name, 0));
}

// FUNCTION: LEGORR 0x0045f230
void reward_load_item_font(RewardLevelItem* item)
{
    char* fontName;

    if ((fontName = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9378
                              "Fonts",
                              item->name, 0))) != NULL) {
        if ((item->font = (struct TextureSurface*)font_atlas_record_create(fontName)) != NULL)
            return;
    }
    item->font = rewardGlobs.font;
}

// FUNCTION: LEGORR 0x0045f290
void reward_load_item_box_image(RewardLevelItem* item)
{
    char* str;
    char* argv[100];
    char name[128];
    char altName[64];

    if ((str = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9388
                              "BoxImages",
                              item->name, 0))) == NULL) {
        if (!(s_engine_feature_word & 0x2000000)) {
            sprintf(altName, "!%s", item->name);
            str = block_get_str_copy(rewardGlobs.config,
                string_path_join(rewardGlobs.gameName, "Reward",
                                 // STRING: LEGORR 0x004a9380
                                 "Images",
                                 altName, 0));
        }
        if (str == NULL) return;
    }

    sound_variant_split(str, argv, "|");
    sprintf(name, "%s", argv[0]);
    item->boxImageX = atoi(argv[1]);
    item->boxImageY = atoi(argv[2]);
    if ((item->boxImage = image_load_bmp_scaled(name, 0, 0)) != NULL) {
        image_setup_trans(item->boxImage, 0, 0, 0, 0, 0, 0);
        item->flags |= REWARD_ITEM_FLAG_BOXIMAGE;
        free(str);
    }
}

// FUNCTION: LEGORR 0x0045f3e0
void reward_load_item_image(RewardLevelItem* item)
{
    char* str;
    char* argv[100];
    char name[128];
    char altName[64];

    if ((str = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward", "Images",
                              item->name, 0))) == NULL) {
        if (!(s_engine_feature_word & 0x2000000)) {
            sprintf(altName, "!%s", item->name);
            str = block_get_str_copy(rewardGlobs.config,
                string_path_join(rewardGlobs.gameName, "Reward", "Images",
                                 altName, 0));
        }
        if (str == NULL) return;
    }

    sound_variant_split(str, argv, "|");
    sprintf(name, "%s", argv[0]);
    item->imageX = atoi(argv[1]);
    item->imageY = atoi(argv[2]);
    if ((item->image = image_load_bmp_scaled(name, 0, 0)) != NULL) {
        image_setup_trans(item->image, 0, 0, 0, 0, 0, 0);
        item->flags |= REWARD_ITEM_FLAG_IMAGE;
        free(str);
    }
}

// FUNCTION: LEGORR 0x0045f530
void reward_load_item_text(RewardLevelItem* item)
{
    char* str;
    char* argv[100];

    if ((str = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward", "Text",
                              item->name, 0))) != NULL) {
        sound_variant_split(str, argv, "|");
        sprintf(item->text, "%s", argv[0]);
        mission_replace_char(item->text, '_', ' ');
        item->textX = atoi(argv[1]);
        item->textY = atoi(argv[2]);
        item->flags |= REWARD_ITEM_FLAG_TEXT | REWARD_ITEM_FLAG_TEXTBOX |
                       REWARD_ITEM_FLAG_TEXTBOXED;
        free(str);
    }
}

// FUNCTION: LEGORR 0x0045f5f0
void reward_load_item_flic(RewardLevelItem* item)
{
    char* str;
    char* argv[100];
    char name[128];
    char altName[64];

    if ((str = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9394
                              "Flics",
                              item->name, 0))) == NULL) {
        if (!(s_engine_feature_word & 0x1000000)) {
            sprintf(altName, "!%s", item->name);
            str = block_get_str_copy(rewardGlobs.config,
                string_path_join(rewardGlobs.gameName, "Reward", "Flics",
                                 altName, 0));
        }
        if (str == NULL) return;
    }

    sound_variant_split(str, argv, "|");
    sprintf(name, "%s", argv[0]);
    sprintf(item->flicName, "%s", name);
    item->flicX = atoi(argv[1]);
    item->flicY = atoi(argv[2]);
    item->flicWidth = atoi(argv[3]);
    item->flicHeight = atoi(argv[4]);
    item->flags |= REWARD_ITEM_FLAG_FLIC;
    free(str);
}

// FUNCTION: LEGORR 0x0045f750
void reward_shutdown(void)
{
    unsigned int i;

    for (i = 0; i < REWARD_ITEM_COUNT; i++) {
        if (rewardGlobs.itemPart->items[i].flags & REWARD_ITEM_FLAG_IMAGE)
            reward_free_item_image(&rewardGlobs.itemPart->items[i]);
        if (rewardGlobs.itemPart->items[i].flags & REWARD_ITEM_FLAG_TEXT)
            reward_free_item_font(&rewardGlobs.itemPart->items[i]);
    }
    free(rewardGlobs.itemPart);
}

// FUNCTION: LEGORR 0x0045f7b0
void reward_free_item_image(RewardLevelItem* item)
{
    image_surface_release(item->image);
}

// FUNCTION: LEGORR 0x0045f7d0
void reward_free_item_font(RewardLevelItem* item)
{
    font_atlas_record_release(item->font);
}

// FUNCTION: LEGORR 0x0045f7f0
int reward_load_graphics(BlockFile* config, const char* gameName)
{
    char* str;
    char* argv[100];

    level_current_owner_get();
    rewardGlobs.config = config;
    rewardGlobs.gameName = gameName;

    rewardGlobs.wallpaper = image_load_bmp_scaled(block_get_strptr(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a9418
                         "Wallpaper",
                         0)), 0, 0);
    if ((rewardGlobs.display = block_get_bool_path(config,
             string_path_join(gameName, "Reward",
                              // STRING: LEGORR 0x004a9410
                              "Display",
                              0))) == 2)
        rewardGlobs.display = 0;
    if ((rewardGlobs.centreText = block_get_bool_path(config,
             string_path_join(gameName, "Reward",
                              // STRING: LEGORR 0x004a9404
                              "CentreText",
                              0))) == 2)
        rewardGlobs.centreText = 0;
    rewardGlobs.scrollSpeed = (float)atof(block_get_strptr(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93f8
                         "ScrollSpeed",
                         0)) ?
        block_get_strptr(config, string_path_join(gameName, "Reward", "ScrollSpeed", 0)) : "");
    rewardGlobs.vertSpacing = (float)atof(block_get_strptr(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93ec
                         "VertSpacing",
                         0)) ?
        block_get_strptr(config, string_path_join(gameName, "Reward", "VertSpacing", 0)) : "");
    rewardGlobs.fontName = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93e4
                         "Font",
                         0));
    rewardGlobs.titleFontName = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93d8
                         "TitleFont",
                         0));
    rewardGlobs.backFontName = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93cc
                         "BackFont",
                         0));
    rewardGlobs.font = (struct TextureSurface*)font_atlas_record_create(rewardGlobs.fontName);
    rewardGlobs.titleFont = (struct TextureSurface*)font_atlas_record_create(rewardGlobs.titleFontName);
    rewardGlobs.backFont = (struct TextureSurface*)font_atlas_record_create(rewardGlobs.backFontName);
    reward_load_buttons(config, gameName);
    rewardGlobs.completeText = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93bc
                         "CompleteText",
                         0));
    rewardGlobs.failedText = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93b0
                         "FailedText",
                         0));
    rewardGlobs.quitText = block_get_str_copy(config,
        string_path_join(gameName, "Reward",
                         // STRING: LEGORR 0x004a93a4
                         "QuitText",
                         0));
    if ((str = block_get_str_copy(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a939c
                              "TextPos",
                              0))) != NULL) {
        sound_variant_split(str, argv, "|");
        rewardGlobs.textPosX = atoi(argv[0]);
        rewardGlobs.textPosY = atoi(argv[1]);
    }
    return 1;
}

// FUNCTION: LEGORR 0x0045faf0
int reward_load_buttons(BlockFile* config, const char* gameName)
{
    char* str;
    char* argv[100];

    if ((str = block_get_str_copy(config,
             string_path_join(gameName, "Reward",
                              // STRING: LEGORR 0x004a9434
                              "SaveButton",
                              0))) != NULL) {
        sound_variant_split(str, argv, "|");
        if ((rewardGlobs.saveButton = image_load_bmp_scaled(argv[0], 0, 0)) != NULL) {
            rewardGlobs.saveButtonHi = image_load_bmp_scaled(argv[1], 0, 0);
            rewardGlobs.saveButtonIn = image_load_bmp_scaled(argv[2], 0, 0);
            rewardGlobs.saveButtonOff = image_load_bmp_scaled(argv[3], 0, 0);
            rewardGlobs.saveButtonPos.x = (float)atoi(argv[4]);
            rewardGlobs.saveButtonPos.y = (float)atoi(argv[5]);
            image_setup_trans(rewardGlobs.saveButton, 0, 0, 0, 0, 0, 0);
            if (rewardGlobs.saveButtonHi)
                image_setup_trans(rewardGlobs.saveButtonHi, 0, 0, 0, 0, 0, 0);
            if (rewardGlobs.saveButtonIn)
                image_setup_trans(rewardGlobs.saveButtonIn, 0, 0, 0, 0, 0, 0);
        }
        free(str);
    }
    if ((str = block_get_str_copy(config,
             string_path_join(gameName, "Reward",
                              // STRING: LEGORR 0x004a9424
                              "AdvanceButton",
                              0))) != NULL) {
        sound_variant_split(str, argv, "|");
        if ((rewardGlobs.advanceButton = image_load_bmp_scaled(argv[0], 0, 0)) != NULL) {
            rewardGlobs.advanceButtonHi = image_load_bmp_scaled(argv[1], 0, 0);
            rewardGlobs.advanceButtonIn = image_load_bmp_scaled(argv[2], 0, 0);
            rewardGlobs.advanceButtonOff = image_load_bmp_scaled(argv[3], 0, 0);
            rewardGlobs.advanceButtonPos.x = (float)atoi(argv[4]);
            rewardGlobs.advanceButtonPos.y = (float)atoi(argv[5]);
            image_setup_trans(rewardGlobs.advanceButton, 0, 0, 0, 0, 0, 0);
            if (rewardGlobs.advanceButtonHi)
                image_setup_trans(rewardGlobs.advanceButtonHi, 0, 0, 0, 0, 0, 0);
            if (rewardGlobs.advanceButtonIn)
                image_setup_trans(rewardGlobs.advanceButtonIn, 0, 0, 0, 0, 0, 0);
        }
        free(str);
    }
    if (rewardGlobs.saveButton && rewardGlobs.advanceButton)
        return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0045fda0
int reward_prepare_level(void)
{
    RewardLegoLevel* level = (RewardLegoLevel*)level_current_owner_get();
    unsigned int i;

    rewardGlobs.current = (RewardLevel*)malloc(sizeof(RewardLevel));
    memset(rewardGlobs.current, 0, sizeof(RewardLevel));

    if ((rewardGlobs.current->enabled = block_get_bool_path(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, level->name, "Reward",
                              // STRING: LEGORR 0x004a9488
                              "Enable",
                              0))) == 2)
        rewardGlobs.current->enabled = 0;
    rewardGlobs.current->modifier = atoi(block_get_strptr(rewardGlobs.config,
        string_path_join(rewardGlobs.gameName, level->name, "Reward",
                         // STRING: LEGORR 0x004a947c
                         "Modifier",
                         0)) ?
        block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, level->name, "Reward", "Modifier", 0)) : "");
    rewardGlobs.timer = (float)atof(block_get_strptr(rewardGlobs.config,
        string_path_join(rewardGlobs.gameName, "Reward", "Timer", 0)) ?
        block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, "Reward", "Timer", 0)) : "");
    if (rewardGlobs.timer == 0.0f)
        rewardGlobs.timer = 76.9375f;
    if ((rewardGlobs.displayText = block_get_bool_path(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9470
                              "DisplayText",
                              0))) == 2)
        rewardGlobs.displayText = 1;
    if ((rewardGlobs.displayImages = block_get_bool_path(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9460
                              "DisplayImages",
                              0))) == 2)
        rewardGlobs.displayImages = 1;
    if ((rewardGlobs.displayFlics = block_get_bool_path(rewardGlobs.config,
             string_path_join(rewardGlobs.gameName, "Reward",
                              // STRING: LEGORR 0x004a9450
                              "DisplayFlics",
                              0))) == 2)
        rewardGlobs.displayFlics = 1;

    rewardGlobs.current->items = rewardGlobs.current->itemsArray;
    rewardGlobs.current->itemCount = REWARD_ITEM_COUNT;
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        rewardGlobs.current->items[i] = rewardGlobs.itemPart->items[i];
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_importance(&rewardGlobs.current->items[i]);
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        reward_load_item_quota(&rewardGlobs.current->items[i]);

    // STRING: LEGORR 0x004a9440
    if (_strcmpi(level->name, "Levels::Level03") == 0)
        rewardGlobs.current->items[REWARD_ITEM_CAVERNS].quota = 1;
    return 1;
}

// FUNCTION: LEGORR 0x004600a0
void reward_load_item_importance(RewardLevelItem* item)
{
    RewardLegoLevel* level = (RewardLegoLevel*)level_current_owner_get();

    if (block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, level->name, "Reward",
                             // STRING: LEGORR 0x004a9490
                             "Importance",
                             item->name, 0)))
        item->importance = (float)atof(block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, level->name, "Reward",
                             "Importance", item->name, 0)));
    else
        item->importance = (float)atof("");
}

// FUNCTION: LEGORR 0x00460140
void reward_load_item_quota(RewardLevelItem* item)
{
    RewardLegoLevel* level = (RewardLegoLevel*)level_current_owner_get();

    if (block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, level->name, "Reward",
                             "Quota", item->name, 0)))
        item->quota = atoi(block_get_strptr(rewardGlobs.config,
            string_path_join(rewardGlobs.gameName, level->name, "Reward",
                             "Quota", item->name, 0)));
    else
        item->quota = atoi("");
}

// FUNCTION: LEGORR 0x004601e0
void reward_free_level(void)
{
    free(rewardGlobs.current);
}

// FUNCTION: LEGORR 0x004601f0
RewardLevel* reward_level_get(void* key)
{
    return rewardGlobs.current;
}

// FUNCTION: LEGORR 0x00460200
RewardLevel* reward_current_level_get(void)
{
    RewardLegoLevel* level = (RewardLegoLevel*)level_current_owner_get();

    if (level == NULL) return NULL;
    return reward_level_get(level->rewardKey);
}

// FUNCTION: LEGORR 0x00460220
int reward_level_start(void)
{
    RewardLegoLevel* level;
    RewardBuildingCounts counts;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if (reward_level_get(level->rewardKey) == NULL) return 0;
    rewardGlobs.level.buildingCounts = *reward_building_counts_get(&counts);
    rewardGlobs.level.itemsArray[REWARD_ITEM_FIGURES].startCount =
        entity_type_grid_record_count_get("Pilot", 0);
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00460290
RewardBuildingCounts* reward_building_counts_get(RewardBuildingCounts* out)
{
    RewardBuildingCounts counts;
    unsigned int x, y;

    counts.count = 0;
    entity_pool_find_by_callback_filtered_current(
        (int (*)(struct EntitySlot*, void*))reward_building_counts_callback, &counts);
    counts.gridCount = 0;
    for (y = 0; y < 15; y++) {
        for (x = 0; x < 16; x++)
            counts.gridCount += grid_record_count_get(4, y, x);
    }
    *out = counts;
    return out;
}

// FUNCTION: LEGORR 0x00460310
int reward_building_counts_callback(void* object, RewardBuildingCounts* counts)
{
    unsigned int type;
    unsigned int index;

    copy_two_dword_pair_to_outputs(object, &type, &index);
    if (type == 4) {
        counts->names[counts->count] = item_name_by_kind_lookup(4, index);
        counts->count++;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00460360
int reward_prepare_results(void)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    unsigned int x, y;
    unsigned int i;
    int predug;
    int pos[2];
    uint32_t crystals;
    uint32_t unusedA;
    uint32_t ore;
    uint32_t unusedB;
    unsigned int timeLimit;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;

    if (rewardGlobs.level.saveHasCapture) {
        memcpy(current->saveCapture, rewardGlobs.level.saveCapture,
               sizeof(current->saveCapture));
        current->saveHasCapture = 1;
    }
    current->itemsArray[REWARD_ITEM_CRYSTALS].total = 0.0f;
    current->itemsArray[REWARD_ITEM_ORE].total = 0.0f;
    current->itemsArray[REWARD_ITEM_DIGGABLE].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_DIGGABLE].actual;
    current->itemsArray[REWARD_ITEM_CAVERNS].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_CAVERNS].actual;
    current->itemsArray[REWARD_ITEM_DIGGABLE].total = 0.0f;
    current->walls.total = 0.0f;
    current->walls.actual = 0.0f;
    current->itemsArray[REWARD_ITEM_FIGURES].count = reward_object_count_get("Pilot", 0, 0);
    current->itemsArray[REWARD_ITEM_FIGURES].startCount =
        rewardGlobs.level.itemsArray[REWARD_ITEM_FIGURES].startCount;
    current->itemsArray[REWARD_ITEM_FIGURES].damage =
        rewardGlobs.level.itemsArray[REWARD_ITEM_FIGURES].damage;
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].count =
        rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].count;
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].startCount =
        rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].startCount;
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].damage =
        (rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].damage >
         (float)(current->itemsArray[REWARD_ITEM_ROCKMONSTERS].count * 100)) ?
        (float)(current->itemsArray[REWARD_ITEM_ROCKMONSTERS].count * 100) :
        rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].damage;
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].lostCount =
        rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].lostCount;
    current->buildingCounts = rewardGlobs.level.buildingCounts;
    current->blockCount = 0;
    current->itemsArray[REWARD_ITEM_OXYGEN].actual =
        ((RewardLegoLevel*)g_active_level_retail)->oxygen;
    current->itemsArray[REWARD_ITEM_OXYGEN].total = 100.0f;
    current->itemsArray[REWARD_ITEM_TIMER].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].actual;
    current->itemsArray[REWARD_ITEM_TIMER].total =
        rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].total /
        current->itemsArray[REWARD_ITEM_TIMER].actual;

    for (x = 0; x < level->map->blockWidth; x++) {
        for (y = 0; y < level->map->blockHeight; y++) {
            predug = level->blocks[y * level->width + x].predug;
            current->blockCount++;
            pos[0] = x;
            pos[1] = y;
            terrain_cell_byte_state_classify_current(pos, &crystals, &unusedA, &ore, &unusedB);
            current->itemsArray[REWARD_ITEM_CRYSTALS].total += (float)crystals;
            current->itemsArray[REWARD_ITEM_ORE].total += (float)ore;
            if (!(level->blocks[y * level->width + x].flags1 & 0x8)) {
                if (predug != 1) {
                    current->itemsArray[REWARD_ITEM_DIGGABLE].total =
                        current->itemsArray[REWARD_ITEM_DIGGABLE].total - -1.0f;
                    current->walls.total = current->walls.total - -1.0f;
                }
                if (predug == 10)
                    current->itemsArray[REWARD_ITEM_CRYSTALS].total =
                        current->itemsArray[REWARD_ITEM_CRYSTALS].total - -4.0f;
            } else if (predug != 7 && predug != 6) {
                current->walls.total = current->walls.total - -1.0f;
            }
            if (level->blocks[y * level->width + x].flags1 & 0x61100000) {
                current->walls.actual = current->walls.actual - -1.0f;
                current->itemsArray[REWARD_ITEM_DIGGABLE].total =
                    current->itemsArray[REWARD_ITEM_DIGGABLE].total - -1.0f;
            }
        }
    }

    current->buildingCount = current->buildingCounts.count;
    current->buildingGridCount = current->buildingCounts.gridCount;
    current->itemsArray[REWARD_ITEM_ORE].total = current->itemsArray[REWARD_ITEM_ORE].total -
        current->itemsArray[REWARD_ITEM_DIGGABLE].total * -4.0f;
    for (i = 0; i < current->buildingCount; i++)
        sprintf(current->buildingNames[i], "%s", current->buildingCounts.names[i]);

    current->itemsArray[REWARD_ITEM_CRYSTALS].actual = (float)level->crystals;
    current->itemsArray[REWARD_ITEM_ORE].actual = (float)level->ore;
    current->itemsArray[REWARD_ITEM_FIGURES].total =
        (float)current->itemsArray[REWARD_ITEM_FIGURES].count;
    current->itemsArray[REWARD_ITEM_FIGURES].actual =
        (float)(current->itemsArray[REWARD_ITEM_FIGURES].count -
                current->itemsArray[REWARD_ITEM_FIGURES].startCount);
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].actual =
        current->itemsArray[REWARD_ITEM_ROCKMONSTERS].damage;
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].total =
        (float)(current->itemsArray[REWARD_ITEM_ROCKMONSTERS].count * 100);
    current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].total = (float)current->buildingCount;
    current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].actual =
        (float)((current->buildingCount - current->buildingGridCount) + 1);

    if ((timeLimit = current->itemsArray[REWARD_ITEM_TIMER].quota) != 0) {
        if ((float)timeLimit > current->itemsArray[REWARD_ITEM_TIMER].actual * 0.04f) {
            current->itemsArray[REWARD_ITEM_TIMER].percent =
                (float)(unsigned int)((100.0f / (float)timeLimit) *
                                      (current->itemsArray[REWARD_ITEM_TIMER].actual * 0.04f));
            current->itemsArray[REWARD_ITEM_TIMER].percent =
                (current->itemsArray[REWARD_ITEM_TIMER].percent > 100.0f) ? 100.0f :
                current->itemsArray[REWARD_ITEM_TIMER].percent;
            current->itemsArray[REWARD_ITEM_TIMER].percent =
                (current->itemsArray[REWARD_ITEM_TIMER].percent < 0.0f) ? 0.0f :
                current->itemsArray[REWARD_ITEM_TIMER].percent;
        } else {
            current->itemsArray[REWARD_ITEM_TIMER].percent = 100.0f;
        }
    } else {
        current->itemsArray[REWARD_ITEM_TIMER].percent = 0.0f;
    }

    if (current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].quota) {
        current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].percent =
            (float)(100 / current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].quota) *
            current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].actual;
    }
    if (current->itemsArray[REWARD_ITEM_CAVERNS].quota) {
        current->itemsArray[REWARD_ITEM_CAVERNS].percent =
            (float)(100 / current->itemsArray[REWARD_ITEM_CAVERNS].quota) *
            current->itemsArray[REWARD_ITEM_CAVERNS].actual;
    }
    if (current->itemsArray[REWARD_ITEM_CRYSTALS].quota)
        current->itemsArray[REWARD_ITEM_CRYSTALS].total =
            (float)current->itemsArray[REWARD_ITEM_CRYSTALS].quota;

    for (i = 0; i < REWARD_ITEM_COUNT; i++) {
        if (i != REWARD_ITEM_TIMER) {
            current->items[i].percent =
                (current->items[i].percent > 100.0f) ? 100.0f : current->items[i].percent;
            current->items[i].percent =
                (current->items[i].percent < 0.0f) ? 0.0f : current->items[i].percent;
        }
    }

    reward_calculate_score();
    reward_create_scroll();
    memset(&rewardGlobs.level, 0, sizeof(rewardGlobs.level));
    return 1;
}

// FUNCTION: LEGORR 0x00460910
int reward_object_count_get(const char* name, int a, int b)
{
    int type;
    int index;

    if (entity_type_pair_lookup(name, &type, &index, 0))
        return grid_record_count_difference_get(type, index, a, b);
    return 0;
}

#include "reward_maths.h"
#define REWARD_PERCENT(item) reward_item_percent(&(item))

/* Add an item's percentage to the score once per point of importance. */
#define REWARD_SCORE(current, item, value) \
    for (i = 0; (float)i < (item).importance; i++) { \
        (current)->itemsArray[REWARD_ITEM_SCORE].actual = \
            (value) + (current)->itemsArray[REWARD_ITEM_SCORE].actual; \
        (current)->itemsArray[REWARD_ITEM_SCORE].total = \
            (current)->itemsArray[REWARD_ITEM_SCORE].total - -100.0f; \
    }

// FUNCTION: LEGORR 0x00460950
int reward_calculate_score(void)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    unsigned int i;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;

    current->itemsArray[REWARD_ITEM_SCORE].actual = 0.0f;
    current->itemsArray[REWARD_ITEM_SCORE].total = 0.0f;
    current->itemsArray[REWARD_ITEM_SCORE].percent = 0.0f;

    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_CRYSTALS]);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_ORE]);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_DIGGABLE]);
    REWARD_PERCENT(current->walls);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_OXYGEN]);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_CONSTRUCTIONS]);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_FIGURES]);
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_ROCKMONSTERS]);

    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_FIGURES],
                 current->itemsArray[REWARD_ITEM_FIGURES].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_CRYSTALS],
                 current->itemsArray[REWARD_ITEM_CRYSTALS].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_OXYGEN],
                 current->itemsArray[REWARD_ITEM_OXYGEN].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_CONSTRUCTIONS],
                 current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_CAVERNS],
                 current->itemsArray[REWARD_ITEM_CAVERNS].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_TIMER],
                 100.0f - current->itemsArray[REWARD_ITEM_TIMER].percent);
    REWARD_SCORE(current, current->itemsArray[REWARD_ITEM_ROCKMONSTERS],
                 current->itemsArray[REWARD_ITEM_ROCKMONSTERS].percent);

    if (current->itemsArray[REWARD_ITEM_SCORE].actual > current->itemsArray[REWARD_ITEM_SCORE].total)
        current->itemsArray[REWARD_ITEM_SCORE].actual = current->itemsArray[REWARD_ITEM_SCORE].total;
    if (current->itemsArray[REWARD_ITEM_SCORE].actual < 0.0f)
        current->itemsArray[REWARD_ITEM_SCORE].actual = 0.0f;
    REWARD_PERCENT(current->itemsArray[REWARD_ITEM_SCORE]);
    current->itemsArray[REWARD_ITEM_SCORE].percent =
        (float)current->modifier + current->itemsArray[REWARD_ITEM_SCORE].percent;
    current->itemsArray[REWARD_ITEM_SCORE].percent =
        (current->itemsArray[REWARD_ITEM_SCORE].percent > 100.0f) ? 100.0f :
        current->itemsArray[REWARD_ITEM_SCORE].percent;
    return 1;
}

// FUNCTION: LEGORR 0x00460f00
int reward_create_scroll(void)
{
    float screenHeight = (float)s_display_h;
    float screenWidth = (float)s_display_w;
    RewardLegoLevel* level;
    Area2F area;

    area.x = 0.0f;
    area.y = screenHeight * 0.05f;
    area.width = screenWidth;
    area.height = screenHeight - screenHeight * 0.2857143f;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if (reward_level_get(level->rewardKey) == NULL) return 0;

    reward_format_values();
    reward_scroll_create(&rewardGlobs.scroll, area, rewardGlobs.scrollSpeed);
    reward_scroll_length_set(rewardGlobs.scroll, 400.0f);
    reward_scroll_flags_set(rewardGlobs.scroll, 0);
    reward_scroll_add_label(rewardGlobs.scroll, level->fullName, rewardGlobs.titleFont,
                            0.0f, (float)s_display_h * 0.04f, 3);
    switch (level->status) {
    case 1:
        sprintf(rewardGlobs.statusText, "%s",
                formatted_text_escape_decode_and_emit(rewardGlobs.completeText));
        break;
    case 2:
        sprintf(rewardGlobs.statusText, "%s",
                formatted_text_escape_decode_and_emit(rewardGlobs.failedText));
        break;
    case 3:
        sprintf(rewardGlobs.statusText, "%s",
                formatted_text_escape_decode_and_emit(rewardGlobs.quitText));
        break;
    }
    reward_scroll_add_label(rewardGlobs.scroll, rewardGlobs.statusText, rewardGlobs.font,
                            0.0f, rewardGlobs.vertSpacing, 1);
    return 1;
}

// FUNCTION: LEGORR 0x00461070
int reward_format_values(void)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    unsigned int time;
    unsigned int value;
    unsigned int n;
    char buff[128];
    char timeText[128];

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;

    current->itemsArray[REWARD_ITEM_CRYSTALS].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_CRYSTALS].percent;
    sprintf(current->itemsArray[REWARD_ITEM_CRYSTALS].percentText,
            // STRING: LEGORR 0x004a94c8
            "%d%%%%",
            current->itemsArray[REWARD_ITEM_CRYSTALS].percentInt);
    current->itemsArray[REWARD_ITEM_ORE].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_ORE].percent;
    sprintf(current->itemsArray[REWARD_ITEM_ORE].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_ORE].percentInt);
    current->itemsArray[REWARD_ITEM_DIGGABLE].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_DIGGABLE].percent;
    sprintf(current->itemsArray[REWARD_ITEM_DIGGABLE].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_DIGGABLE].percentInt);
    current->walls.percentInt = (unsigned int)current->walls.percent;
    sprintf(current->walls.percentText,
            // STRING: LEGORR 0x004a94c4
            "%d",
            current->walls.percentInt);
    current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].percentInt = current->buildingCount;
    sprintf(current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].percentText, "%d",
            current->itemsArray[REWARD_ITEM_CONSTRUCTIONS].percentInt);
    current->itemsArray[REWARD_ITEM_CAVERNS].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_CAVERNS].percent;
    sprintf(current->itemsArray[REWARD_ITEM_CAVERNS].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_CAVERNS].percentInt);
    current->itemsArray[REWARD_ITEM_FIGURES].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_FIGURES].percent;
    sprintf(current->itemsArray[REWARD_ITEM_FIGURES].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_FIGURES].percentInt);
    current->itemsArray[REWARD_ITEM_ROCKMONSTERS].percentInt = (unsigned int)
        ((current->itemsArray[REWARD_ITEM_ROCKMONSTERS].damage < 100.0f) ?
         current->itemsArray[REWARD_ITEM_ROCKMONSTERS].damage : 100.0f);
    sprintf(current->itemsArray[REWARD_ITEM_ROCKMONSTERS].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_ROCKMONSTERS].percentInt);
    current->itemsArray[REWARD_ITEM_OXYGEN].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_OXYGEN].percent;
    sprintf(current->itemsArray[REWARD_ITEM_OXYGEN].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_OXYGEN].percentInt);

    time = current->itemsArray[REWARD_ITEM_TIMER].percentInt = (unsigned int)
        (current->itemsArray[REWARD_ITEM_TIMER].actual *
         current->itemsArray[REWARD_ITEM_TIMER].total * 0.04f);
    time %= 359999;
    sprintf(timeText, "");
    if (time > 3600) {
        n = (time - 1) / 3600;
        value = n;
        do time -= 3600; while (--n);
        if (value < 10)
            // STRING: LEGORR 0x004a94bc
            sprintf(buff, "0%d:", value);
        else
            // STRING: LEGORR 0x004a94b8
            sprintf(buff, "%d:", value);
        strcat(timeText, buff);
    } else {
        // STRING: LEGORR 0x004a94b4
        strcat(timeText, "00:");
    }
    if (time > 60) {
        n = (time - 1) / 60;
        value = n;
        do time -= 60; while (--n);
        if (value < 10)
            // STRING: LEGORR 0x004a94ac
            sprintf(buff, "0%d.", value);
        else
            // STRING: LEGORR 0x004a94a8
            sprintf(buff, "%d.", value);
        strcat(timeText, buff);
    } else {
        // STRING: LEGORR 0x004a94a4
        strcat(timeText, "00.");
    }
    if (time > 1) {
        if (time - 1 < 10)
            // STRING: LEGORR 0x004a94a0
            sprintf(buff, "0%d", time - 1);
        else
            sprintf(buff, "%d", time - 1);
        strcat(timeText, buff);
    } else {
        // STRING: LEGORR 0x004a949c
        strcat(timeText, "00");
    }
    sprintf(current->itemsArray[REWARD_ITEM_TIMER].percentText, "%s", timeText);

    current->itemsArray[REWARD_ITEM_SCORE].percentInt =
        (unsigned int)current->itemsArray[REWARD_ITEM_SCORE].percent;
    sprintf(current->itemsArray[REWARD_ITEM_SCORE].percentText, "%d%%%%",
            current->itemsArray[REWARD_ITEM_SCORE].percentInt);
    return 1;
}

// FUNCTION: LEGORR 0x00461410
int reward_loop(void)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    int count = 0;
    int state = 0;
    int drawn = 0;
    int saved = 0;
    int i;
    int n;
    float time;
    float last;
    float elapsed;
    const char* movieName;
    struct DirectShowGraphState* movie;

    count = 0;
    last = (float)timeGetTime();
    time = rewardGlobs.timer;
    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;
    if (!reward_check_skip()) return 0;

    while (state < 4 || g_renderer_word_c || s_window_mouse_word_4c) {
        elapsed = ((float)timeGetTime() - last) * 0.001f;
        last = (float)timeGetTime();
        time += elapsed;
        reward_update_state(&count, &state, &time);
        if (((!s_global_current_keys[0x39] && s_global_previous_keys[0x39]) ||
             s_window_mouse_word_4c) && state < 3) {
            if (++state == 3) state = 2;
            if (state > 1 && level->status == 1)
                count = 10;
            else
                count = 9;
        }
        if (rewardGlobs.wallpaper)
            image_display_scaled(rewardGlobs.wallpaper, NULL, NULL, NULL);
        for (n = 0; n < count; n++)
            reward_draw_item(&current->items[n], REWARD_ITEM_FLAG_IMAGE, count, time);
        for (n = 0; n < ((count < 10) ? count : 9); n++)
            reward_draw_item(&current->items[n], REWARD_ITEM_FLAG_BOXIMAGE, count, time);
        for (n = 0; n < count; n++)
            reward_draw_item(&current->items[n], REWARD_ITEM_FLAG_FLIC, count, time);
        reward_draw_values(&count, &drawn, time);
        if (drawn) {
            reward_draw_score(count, time);
            if (state < 3)
                reward_do_buttons(&state, &saved);
        }
        reward_scroll_update(rewardGlobs.scroll);
        if (state == 3) state = 4;
        reward_debug_keys(&count, &state, &drawn, &time);
        reward_update_input(elapsed);
    }

    reward_scroll_free(&rewardGlobs.scroll);
    for (i = 0; i < REWARD_ITEM_COUNT; i++) {
        if (current->items[i].flags & REWARD_ITEM_FLAG_FLIC) {
            flic_close(current->items[i].flic);
            current->items[i].flic = NULL;
        }
    }
    sound_stream_stop_and_release(1);

    if (level->status == 1 && g_front_end_avi1) {
        if (!upgrade_average_is_above_99() || (movieName = g_front_end_avi2) == NULL)
            movieName = g_front_end_avi1;
        if (sound_sample_file_open_by_name(movieName)) {
            movie = directshow_render_surface_create(movieName);
            movie_play_wait(movie, 0);
            directshow_wrapper_destroy(movie);
        }
        sound_request_queue_submit(0x2c, 1);
        main_menu_text_item();
        sound_stream_stop_and_release(1);
    }
    return 1;
}

// FUNCTION: LEGORR 0x00461790
void reward_draw_item(RewardLevelItem* item, unsigned int mask, unsigned int count,
                      float time)
{
    RewardLevelItem* last;
    unsigned int width;

    last = &rewardGlobs.current->items[count ? count - 1 : 0];

    if (rewardGlobs.displayImages && (item->flags & REWARD_ITEM_FLAG_IMAGE) &&
        (mask & REWARD_ITEM_FLAG_IMAGE)) {
        Point2F pos;
        pos.x = (float)item->imageX;
        pos.y = (float)item->imageY;
        image_display_scaled(item->image, NULL, &pos, NULL);
    }
    if (rewardGlobs.displayImages && (item->flags & REWARD_ITEM_FLAG_BOXIMAGE) &&
        (mask & REWARD_ITEM_FLAG_BOXIMAGE)) {
        Point2F pos;
        pos.x = (float)item->boxImageX;
        pos.y = (float)item->boxImageY;
        image_display_scaled(item->boxImage, NULL, &pos, NULL);
    }
    if (rewardGlobs.displayFlics && (item->flags & REWARD_ITEM_FLAG_FLIC) &&
        (mask & REWARD_ITEM_FLAG_FLIC)) {
        Area2F rect;
        if (item->flic == NULL)
            flic_setup(item->flicName, (void**)&item->flic, 0);
        rect.x = (float)item->flicX;
        rect.y = (float)item->flicY;
        rect.width = (float)item->flicWidth;
        rect.height = (float)item->flicHeight;
        flic_animate(item->flic, &rect, (REWARD_FLIC_FRAME(item->flic) < 35) ? 1 : 0, 0);
    }
    if (rewardGlobs.displayText) {
        if ((item->flags & REWARD_ITEM_FLAG_TEXT) && (mask & REWARD_ITEM_FLAG_TEXT)) {
            width = formatted_text_printf_width(item->font, item->percentText);
            font_print_f(item->font, item->textX - (width >> 1), item->textY, item->percentText);
            if (count < 10) {
                width = formatted_text_printf_width(rewardGlobs.titleFont, last->text);
                font_print_f(rewardGlobs.titleFont, rewardGlobs.textPosX - (width >> 1),
                             rewardGlobs.textPosY, last->text);
            }
        }
        if (rewardGlobs.displayText &&
            (((item->flags & REWARD_ITEM_FLAG_TEXTBOX) && (mask & REWARD_ITEM_FLAG_TEXTBOX)) ||
             ((item->flags & REWARD_ITEM_FLAG_TEXTBOXED) && (mask & REWARD_ITEM_FLAG_TEXTBOXED)))) {
            ColourRGBF fill;
            ColourRGBF highlight;
            ColourRGBF shadow;
            Area2F rect;
            fill.red = 0.11764706f;
            fill.green = 0.11764706f;
            fill.blue = 0.11764706f;
            highlight.red = 0.23529412f;
            highlight.green = 0.23529412f;
            highlight.blue = 0.23529412f;
            shadow.red = 0.0f;
            shadow.green = 0.0f;
            shadow.blue = 0.0f;
            width = formatted_text_printf_width(item->font, item->percentText);
            rect.x = (float)((item->textX - (width >> 1)) - 10);
            rect.y = (float)item->textY;
            rect.width = (float)(width + 20);
            rect.height = (float)object_field_be4_dword_get(item->font);
            tooltip_draw_box(rect, fill, highlight, shadow, mask & REWARD_ITEM_FLAG_TEXTBOXED);
            font_print_f(item->font, item->textX - (width >> 1), item->textY, item->percentText);
            if (count < 10) {
                width = formatted_text_printf_width(rewardGlobs.titleFont, last->text);
                rect.x = (float)((rewardGlobs.textPosX - (width >> 1)) - 10);
                rect.y = (float)rewardGlobs.textPosY;
                rect.width = (float)(width + 20);
                rect.height = (float)object_field_be4_dword_get(rewardGlobs.titleFont);
                tooltip_draw_box(rect, fill, highlight, shadow, 0);
                font_print_f(rewardGlobs.titleFont, rewardGlobs.textPosX - (width >> 1),
                             rewardGlobs.textPosY, last->text);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00461c90
int reward_draw_score(unsigned int count, float time)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    char name[32];
    char value[32];
    int i;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;
    if (level->status == 1) {
        reward_draw_item(&current->items[REWARD_ITEM_SCORE], REWARD_ITEM_FLAG_TEXT, count, time);
        sprintf(name, "%s", current->items[REWARD_ITEM_SCORE].text);
        for (i = 0; i < (int)strlen(current->items[REWARD_ITEM_SCORE].text) - 1; i++) {
            if (current->items[REWARD_ITEM_SCORE].text[i] == ' ') {
                name[i] = '\0';
                sprintf(value, "%s", &name[i + 1]);
            }
        }
        font_print_f(rewardGlobs.backFont,
                     current->items[REWARD_ITEM_SCORE].textX -
                         (formatted_text_printf_width(rewardGlobs.backFont, name) >> 1),
                     current->items[REWARD_ITEM_SCORE].textY - 40, name);
    }
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00461dd0
int reward_do_buttons(int* state, int* saved)
{
    RewardLegoLevel* level;
    Image* image;
    int clicked = 0;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if (reward_level_get(level->rewardKey) == NULL) return 0;

    if (level->status == 1) {
        if ((float)REWARD_MOUSE_X > rewardGlobs.saveButtonPos.x &&
            (float)REWARD_MOUSE_X < (float)rewardGlobs.saveButton->width + rewardGlobs.saveButtonPos.x &&
            (float)REWARD_MOUSE_Y > rewardGlobs.saveButtonPos.y &&
            (float)REWARD_MOUSE_Y < (float)rewardGlobs.saveButton->height + rewardGlobs.saveButtonPos.y) {
            timed_effect_update_request_set_corpus(0x24);
            if ((g_renderer_word_c && (image = rewardGlobs.saveButtonIn) != NULL) ||
                (image = rewardGlobs.saveButtonHi) != NULL)
                image_display_scaled(image, NULL, &rewardGlobs.saveButtonPos, NULL);
            if (ledger_stub_00435550_GlobalStateZeroTransitionDetect()) {
                clicked = 1;
                sound_alias_play(0x13, 0);
                reward_save_menu();
                *saved = 1;
                sound_handle_stop_forward(0x29);
                sound_request_queue_submit(0x29, 1);
            }
        } else {
            image_display_scaled(rewardGlobs.saveButton, NULL, &rewardGlobs.saveButtonPos, NULL);
        }
    } else if (rewardGlobs.saveButtonOff) {
        image_display_scaled(rewardGlobs.saveButtonOff, NULL, &rewardGlobs.saveButtonPos, NULL);
    }

    if ((float)REWARD_MOUSE_X > rewardGlobs.advanceButtonPos.x &&
        (float)REWARD_MOUSE_X < (float)rewardGlobs.advanceButton->width + rewardGlobs.advanceButtonPos.x &&
        (float)REWARD_MOUSE_Y > rewardGlobs.advanceButtonPos.y &&
        (float)REWARD_MOUSE_Y < (float)rewardGlobs.advanceButton->height + rewardGlobs.advanceButtonPos.y) {
        timed_effect_update_request_set_corpus(0x25);
        if ((g_renderer_word_c && (image = rewardGlobs.advanceButtonIn) != NULL) ||
            (image = rewardGlobs.advanceButtonHi) != NULL)
            image_display_scaled(image, NULL, &rewardGlobs.advanceButtonPos, NULL);
        if (ledger_stub_00435550_GlobalStateZeroTransitionDetect()) {
            clicked = 1;
            *state = 3;
            sound_alias_play(0x13, 0);
            reward_advance();
        }
    } else {
        image_display_scaled(rewardGlobs.advanceButton, NULL, &rewardGlobs.advanceButtonPos, NULL);
    }

    if (!clicked && ledger_stub_00435550_GlobalStateZeroTransitionDetect())
        sound_alias_play(0x14, 0);
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00462030
void reward_save_menu(void)
{
    RewardLegoLevel* level = (RewardLegoLevel*)level_current_owner_get();
    int inGame;
    int slot;
    void* save;
    RewardLevel* current;
    uint32_t render[6];
    uint32_t snapshot[46];

    save_slot_open();
    inGame = is_in_game();
    options_write_to_slot_target();
    if (frontend_enabled(rewardGlobs.config)) {
        sav_record_ptr_target(REWARD_LEVEL_INDEX(level->name));
        slot = save_slot_get_target();
        save = sav_build_ctx_target();
        front_campaign_snapshot_create(snapshot);
        current = reward_current_level_get();
        if (!inGame && level_objective_result_get((const struct LevelState*)level)) {
            do {
                menu_enter(NULL, 4);
            } while (g_front_save_menu_open);
            if (g_front_save_menu_result == 1) {
                if (current_selection_context_get()) {
                    show_level_overlay_target();
                    current_selection_context_set(NULL);
                }
                if (sav_build_ctx_target()) {
                    if (sav_build_ctx_target() != save && save)
                        options_context_copy_and_replace_resource_target(snapshot);
                    save_mark_record(NULL, REWARD_LEVEL_INDEX(
                        ((RewardLegoLevel*)level_current_owner_get())->name));
                    save_write_record_target(REWARD_LEVEL_INDEX(level->name),
                                             (const uint8_t*)current);
                    render_config_six_field_copy_out(render);
                    options_context_six_field_set_target(render);
                    save_commit(NULL);
                } else {
                    REWARD_LEVEL_INDEX(((RewardLegoLevel*)level_current_owner_get())->name);
                }
            } else {
                save_slot_set_target(slot);
                if (sav_build_ctx_target()) {
                    save_mark_record(NULL, REWARD_LEVEL_INDEX(
                        ((RewardLegoLevel*)level_current_owner_get())->name));
                    save_write_record_target(REWARD_LEVEL_INDEX(level->name),
                                             (const uint8_t*)current);
                    render_config_six_field_copy_out(render);
                    options_context_six_field_set_target(render);
                }
            }
        }
    }
}

/* FrontEnd.c's save record (SaveData, partial). */
typedef struct RewardSaveData {
    unsigned char unk_00[0x18];
    unsigned int rewardCount;   /* +0x18 */
    unsigned char unk_1c[0x18];
    unsigned char* rewards;     /* +0x34 0x3190-byte records */
} RewardSaveData;

// FUNCTION: LEGORR 0x00462270
void reward_advance(void)
{
    int index;
    int created;
    RewardSaveData* data;
    void**** menuSet;

    index = REWARD_LEVEL_INDEX(((RewardLegoLevel*)level_current_owner_get())->name);
    created = 0;
    if (save_slot_get_target() == -1) {
        save_slot_set_target(5);
        created = 1;
    }
    if (((RewardLegoLevel*)level_current_owner_get())->status == 1)
        save_mark_record(NULL, index);
    save_write_record_target(index, (const uint8_t*)reward_current_level_get());
    if (created) {
        if ((data = (RewardSaveData*)sav_slot_ctx_target(5)) != NULL) {
            memset(data->rewards + 0x3190, 0, data->rewardCount * 0x3190 - 0x3190);
            if (((RewardLegoLevel*)level_current_owner_get())->status == 1)
                save_mark_record(NULL, index);
            save_write_record_target(index, (const uint8_t*)reward_current_level_get());
        }
        save_commit(NULL);
        save_read_slot_target(5, g_front_save_slot_buffer, 0);
    }
    menuSet = (void****)g_front_main_menu_set;
    sav_tree_load(g_front_save_tree,
                  ((RewardSaveData*)sav_build_ctx_target())->rewards,
                  (void**)active_object_list_word(),
                  *(uint32_t*)((char*)((void**)(*(void***)((*menuSet)[1]))[6])[1] + 0x10), 0, NULL);
    save_state_event_context_set(0);
}

// FUNCTION: LEGORR 0x00462390
int reward_debug_keys(int* count, int* state, int* drawn, float* time)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    int i;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;
    if (s_panel_highlight_flags & 0x10) {
        if (s_global_current_keys[0x39] && s_global_current_keys[0x01] &&
            (s_global_previous_keys[0x01] ^ s_global_current_keys[0x01]))
            lego_reset_level_and_exit();
        if (s_global_current_keys[0x13] &&
            (s_global_previous_keys[0x13] ^ s_global_current_keys[0x13])) {
            *count = 0;
            *state = 0;
            *time = rewardGlobs.timer;
            *drawn = 0;
            sound_handle_stop_forward(0x29);
            sound_request_queue_submit(0x29, 1);
            for (i = 0; i < REWARD_ITEM_COUNT; i++)
                reward_restart_flic(&current->items[i]);
        }
    }
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00462460
void reward_restart_flic(RewardLevelItem* item)
{
    if (item->flags & REWARD_ITEM_FLAG_FLIC) {
        flic_close(item->flic);
        flic_setup(item->flicName, (void**)&item->flic, 0);
    }
}

// FUNCTION: LEGORR 0x004624a0
int reward_update_state(int* count, int* state, float* time)
{
    int next = 0;
    RewardLegoLevel* level;
    RewardLevel* current;
    int sound;
    int i;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;

    if (*time >= rewardGlobs.timer && *state == 0)
        next = 1;
    if (*state != 0 && *time >= rewardGlobs.timer + rewardGlobs.timer &&
        ((RewardLegoLevel*)level_current_owner_get())->status == 1 && *count < 10)
        next = 1;

    if (next) {
        *time -= rewardGlobs.timer;
        *count = (*count < 10) ? *count + 1 : *count;
        if (current->items[*count - 1].soundName &&
            sound_alias_lookup(current->items[*count - 1].soundName, &sound))
            sound_request_queue_submit(sound, 0);
        if (*count > 9) {
            if (*state == 0) {
                *count = 9;
                (*state)++;
                return *state;
            }
            if (*count > 9 && *state > 1) {
                if (*count > 10) {
                    *count = 10;
                    (*state)++;
                    return *state;
                }
                for (i = 0; i < REWARD_ITEM_COUNT; i++)
                    reward_restart_flic(&current->items[i]);
            }
        }
    }
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00462600
int reward_draw_values(int* count, int* drawn, float time)
{
    RewardLegoLevel* level;
    RewardLevel* current;
    int i;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) == NULL) return 0;
    if ((current = reward_level_get(level->rewardKey)) == NULL) return 0;

    if ((*count == 10 || (level->status != 1 && *count == 9)) &&
        (current->itemsArray[REWARD_ITEM_SCORE].flic == NULL || !rewardGlobs.displayFlics ||
         REWARD_FLIC_FRAME(current->items[REWARD_ITEM_SCORE].flic) > 33 || level->status != 1))
        *drawn = 1;
    for (i = 0; i < ((*count < 10) ? *count : 9); i++)
        reward_draw_item(&current->items[i], REWARD_ITEM_FLAG_TEXT, *count, time);
    formatted_text_printf_width(rewardGlobs.titleFont, level->fullName);
    formatted_text_printf_width(rewardGlobs.titleFont, rewardGlobs.statusText);
#if !defined(_MSC_VER)
    return 1;
#endif
}

// FUNCTION: LEGORR 0x00462700
void reward_update_input(float elapsed)
{
    media_pointer_cursor_override_tick(1.0f);
    media_pointer_cursor_select(0);
    pointer_cursor_draw_target(s_renderer_word_a, s_renderer_word_b);
    ledger_stub_00435550_GlobalStateZeroTransitionDetect();
    s_window_mouse_word_4c = 0;
    sound_subsystem_frame_tick(elapsed);
    timed_effect_runtime_update(s_renderer_word_a, s_renderer_word_b, (elapsed * 0.04f) * 1000.0f);
    main_loop_frame_pump(1);
}

// FUNCTION: LEGORR 0x00462780
int reward_check_skip(void)
{
    RewardLegoLevel* level;
    RewardLevel* current;

    if ((level = (RewardLegoLevel*)level_current_owner_get()) != NULL &&
        (current = reward_level_get(level->rewardKey)) != NULL) {
        if (rewardGlobs.display && current->enabled) {
            while ((s_global_current_keys[0x39] &&
                    (s_global_previous_keys[0x39] ^ s_global_current_keys[0x39])) ||
                   s_global_current_keys[0x39] ||
                   s_global_previous_keys[0x39] ||
                   (s_global_previous_keys[0x39] ^ s_global_current_keys[0x39]))
                main_loop_frame_pump(1);
            s_window_mouse_word_4c = 0;
            sound_request_queue_submit(0x29, 1);
            return 1;
        }
        reward_advance();
    }
    return 0;
}

// FUNCTION: LEGORR 0x00462800
void reward_score_add(float value)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].actual - -1.0f;
    rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].total += value;
}

// FUNCTION: LEGORR 0x00462830
void global_float_at_00554adc_add_one(void)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_DIGGABLE].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_DIGGABLE].actual - -1.0f;
}

// FUNCTION: LEGORR 0x00462850
void global_float_at_00554efc_add_one(void)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_CAVERNS].actual =
        rewardGlobs.level.itemsArray[REWARD_ITEM_CAVERNS].actual - -1.0f;
}

// FUNCTION: LEGORR 0x00462870
void global_counter_at_00555324_increment(void)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].count++;
}

// FUNCTION: LEGORR 0x00462880
void global_counter_at_00555328_increment(void)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].startCount++;
}

// FUNCTION: LEGORR 0x00462890
void global_counter_at_0055532c_increment(void)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].lostCount++;
}

// FUNCTION: LEGORR 0x004628a0
void global_float_accumulator_00555330_add(float value)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_ROCKMONSTERS].damage += value;
}

// FUNCTION: LEGORR 0x004628c0
void global_float_accumulate_00555120(float value)
{
    rewardGlobs.level.itemsArray[REWARD_ITEM_FIGURES].damage += value;
}

// FUNCTION: LEGORR 0x004628e0
RewardScroll* reward_scroll_create(RewardScroll** out, Area2F area, float speed)
{
    RewardScroll* scroll;
    RewardScrollLabel** labels;
    RewardScrollLabel* label;

    if ((scroll = (RewardScroll*)malloc(sizeof(RewardScroll))) != NULL) {
        memset(scroll, 0, sizeof(RewardScroll));
        scroll->labels = labels = (RewardScrollLabel**)malloc(sizeof(RewardScrollLabel*));
        labels[0] = label = (RewardScrollLabel*)malloc(sizeof(RewardScrollLabel));
        memset(label, 0, sizeof(RewardScrollLabel));
        scroll->speed = speed;
        scroll->area = area;
        scroll->top = area.y;
    }
    *out = scroll;
    return scroll;
}

// FUNCTION: LEGORR 0x00462960
int reward_scroll_free(RewardScroll** scroll)
{
    unsigned int i;

    for (i = 0; i < (*scroll)->labelCount; i++) {
        if ((*scroll)->labels[i]->text)
            free((*scroll)->labels[i]->text);
        free((*scroll)->labels[i]);
    }
    if ((*scroll)->labels)
        free((*scroll)->labels);
    free(*scroll);
    return 1;
}

// FUNCTION: LEGORR 0x004629d0
RewardScrollLabel* reward_scroll_add_label(RewardScroll* scroll, const char* text,
                                           struct TextureSurface* font, float x, float y,
                                           unsigned int flags)
{
    unsigned int index;

    if (font == NULL) return NULL;
    index = scroll->labelCount++;
    scroll->labels = (RewardScrollLabel**)realloc(scroll->labels,
                                                  scroll->labelCount * sizeof(RewardScrollLabel*));
    scroll->labels[index] = (RewardScrollLabel*)malloc(sizeof(RewardScrollLabel));
    memset(scroll->labels[index], 0, sizeof(RewardScrollLabel));
    scroll->labels[index]->x = x;
    scroll->labels[index]->y = y;
    scroll->labels[index]->text = (char*)malloc(strlen(text) + 1);
    sprintf(scroll->labels[index]->text, "%s", text);
    scroll->labels[index]->type = 3;
    scroll->labels[index]->font = font;
    scroll->labels[index]->flags |= flags;
    return scroll->labels[index];
}

// FUNCTION: LEGORR 0x00462ab0
void reward_scroll_length_set(RewardScroll* scroll, float length)
{
    scroll->length = length;
}

// FUNCTION: LEGORR 0x00462ac0
void reward_scroll_flags_set(RewardScroll* scroll, unsigned int flags)
{
    scroll->flags |= flags;
}

// FUNCTION: LEGORR 0x00462ad0
int reward_scroll_update(RewardScroll* scroll)
{
    RewardScrollLabel* label;
    unsigned int i;
    int x;
    int y;
    int n;
    int k;
    float bottom;

    for (i = 0; i < scroll->labelCount; i++) {
        label = scroll->labels[i];
        x = (int)(label->x + scroll->area.x);
        y = (int)(label->y + scroll->area.y);
        if (scroll->flags & 1) {
            if ((float)y < scroll->top) {
                n = (int)(scroll->area.height + scroll->length);
                do y += n; while ((float)y < scroll->top);
            }
            bottom = scroll->length + scroll->top + scroll->area.height;
            if ((float)y > bottom) {
                n = (int)scroll->area.height;
                do y -= n; while ((float)y > bottom);
            }
        }
        switch (label->type) {
        case 1:
            float_pair_at_offset4_set_from_ints(label->textWindow, x, y);
            text_buffer_draw_wrapped(scroll->labels[i]->textWindow, 0, 1.0f, NULL);
            /* fall through */
        case 2:
            {
                Point2F pos;
                Point2F size;
                pos.x = (float)x;
                pos.y = (float)y;
                size.x = (float)scroll->labels[i]->image->width;
                size.y = (float)scroll->labels[i]->image->height;
                image_display_scaled(scroll->labels[i]->image, NULL, &pos, &size);
            }
            /* fall through */
        case 3:
            label = scroll->labels[i];
            if (label->flags & 1) {
                n = formatted_text_printf_width(label->font, label->text);
                x += -(int)((unsigned int)n >> 1) - (int)((scroll->area.width - scroll->area.x) * -0.5f);
            }
            label = scroll->labels[i];
            if (label->flags & 2)
                y = (int)label->y;
            if ((label->flags & 2) ||
                ((float)y < scroll->area.height + scroll->top && (float)y > scroll->top))
                font_print_f(label->font, x, y, label->text);
            break;
        }
    }

    scroll->area.y = scroll->area.y - scroll->speed;
    if (s_panel_highlight_flags & 0x10) {
        if (s_global_current_keys[0x1f]) scroll->speed = 0.0f;
        if (s_global_current_keys[0x13]) scroll->area.y = scroll->top;
        if (s_global_current_keys[0x0d]) scroll->speed = scroll->speed - -0.01f;
        if (s_global_current_keys[0x0c]) scroll->speed = scroll->speed - 0.01f;
        for (k = 0; k < 10; k++) {
            if (s_global_current_keys[0x02 + k])
                scroll->speed = (float)((scroll->speed < 0.0f) ? -k : k) * 0.1f;
        }
    }
    return 1;
}
