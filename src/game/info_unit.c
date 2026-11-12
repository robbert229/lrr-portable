// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-019: the in-game information messages (the original's Info.c),
 * rebuilt in original shape (ADR 0005, finding 567). Text
 * 00419230-0041a300. See info_unit.h for the state and the port's split. */
#include <stdlib.h>
#include <string.h>

#include "info_unit.h"
#include "info.h"
#include "info_api.h"
#include "crt/vc5names.h"
#include "gods98/images.h"
#include "gods98/font_api.h"
#include "advisor.h"
#include "panel_unit.h"
#include "interface_unit.h"
#include "tutorial_script.h"

/* Panel.c's information panel. */
#define INFO_PANEL 7

/* The search records the enumeration callbacks take. */
typedef struct InfoSearchObject {
    void* object;
    int index;
} InfoSearchObject;

typedef struct InfoSearchBlock {
    InfoPoint2I* blockPos;
    int index;
} InfoSearchBlock;

typedef struct InfoSearchDuplicate {
    void* object;
    InfoPoint2I* blockPos;
} InfoSearchDuplicate;

// GLOBAL: LEGORR 0x004dc658
static InfoGlobs infoGlobs;

// FUNCTION: LEGORR 0x00419230
void info_initialise(void* level)
{
    // STRING: LEGORR 0x004a2b40
    infoGlobs.infoName[0] = "Info_CrystalFound";
    infoGlobs.level = level;
    // STRING: LEGORR 0x004a2b2c
    infoGlobs.infoName[1] = "Info_OreSeamFound";
    // STRING: LEGORR 0x004a2b18
    infoGlobs.infoName[2] = "Info_RockMonster";
    // STRING: LEGORR 0x004a2b00
    infoGlobs.infoName[3] = "Info_LavaRockMonster";
    // STRING: LEGORR 0x004a2aec
    infoGlobs.infoName[4] = "Info_IceRockMonster";
    // STRING: LEGORR 0x004a2ad8
    infoGlobs.infoName[5] = "Info_UnderAttack";
    // STRING: LEGORR 0x004a2ac8
    infoGlobs.infoName[6] = "Info_Landslide";
    // STRING: LEGORR 0x004a2abc
    infoGlobs.infoName[7] = "Info_CaveIn";
    // STRING: LEGORR 0x004a2aa8
    infoGlobs.infoName[8] = "Info_Constructed";
    // STRING: LEGORR 0x004a2a94
    infoGlobs.infoName[9] = "Info_CavernLocated";
    // STRING: LEGORR 0x004a2a80
    infoGlobs.infoName[10] = "Info_LegoManDeath";
    // STRING: LEGORR 0x004a2a6c
    infoGlobs.infoName[11] = "Info_VehicleDeath";
    // STRING: LEGORR 0x004a2a58
    infoGlobs.infoName[12] = "Info_BuildingDeath";
    // STRING: LEGORR 0x004a2a44
    infoGlobs.infoName[13] = "Info_DynamitePlaced";
    // STRING: LEGORR 0x004a2a34
    infoGlobs.infoName[14] = "Info_NoPower";
    // STRING: LEGORR 0x004a2a24
    infoGlobs.infoName[15] = "Info_PowerDrain";
    // STRING: LEGORR 0x004a2a10
    infoGlobs.infoName[16] = "Info_AirDepleting";
    // STRING: LEGORR 0x004a2a04
    infoGlobs.infoName[17] = "Info_AirLow";
    // STRING: LEGORR 0x004a29f8
    infoGlobs.infoName[18] = "Info_AirOut";
    // STRING: LEGORR 0x004a29e4
    infoGlobs.infoName[19] = "Info_AirRestored";
    // STRING: LEGORR 0x004a29d0
    infoGlobs.infoName[20] = "Info_TrainDriver";
    // STRING: LEGORR 0x004a29bc
    infoGlobs.infoName[21] = "Info_TrainDynamite";
    // STRING: LEGORR 0x004a29a8
    infoGlobs.infoName[22] = "Info_TrainRepair";
    // STRING: LEGORR 0x004a2998
    infoGlobs.infoName[23] = "Info_TrainPilot";
    // STRING: LEGORR 0x004a2984
    infoGlobs.infoName[24] = "Info_TrainSailor";
    // STRING: LEGORR 0x004a2970
    infoGlobs.infoName[25] = "Info_TrainScanner";
    // STRING: LEGORR 0x004a295c
    infoGlobs.infoName[26] = "Info_OreCollected";
    // STRING: LEGORR 0x004a294c
    infoGlobs.infoName[27] = "Info_WallDug";
    // STRING: LEGORR 0x004a2938
    infoGlobs.infoName[28] = "Info_WallReinforced";
    // STRING: LEGORR 0x004a2924
    infoGlobs.infoName[29] = "Info_CrystalPower";
    // STRING: LEGORR 0x004a2914
    infoGlobs.infoName[30] = "Info_LavaErode";
    // STRING: LEGORR 0x004a2904
    infoGlobs.infoName[31] = "Info_SlugEmerge";
    // STRING: LEGORR 0x004a28f0
    infoGlobs.infoName[32] = "Info_PathCompleted";
    // STRING: LEGORR 0x004a28d8
    infoGlobs.infoName[33] = "Info_FoundMinifigure";
    // STRING: LEGORR 0x004a28bc
    infoGlobs.infoName[34] = "Info_CanUpgradeMinifigure";
    // STRING: LEGORR 0x004a28a4
    infoGlobs.infoName[35] = "Info_CanTrainMinifigure";
    // STRING: LEGORR 0x004a288c
    infoGlobs.infoName[36] = "Info_CrystalSeamFound";
    // STRING: LEGORR 0x004a2874
    infoGlobs.infoName[37] = "Info_GenericSeamFound";
    // STRING: LEGORR 0x004a2860
    infoGlobs.infoName[38] = "Info_GenericDeath";
    // STRING: LEGORR 0x004a284c
    infoGlobs.infoName[39] = "Info_GenericMonster";
    infoGlobs.flags |= INFO_FLAG_SOUNDS | INFO_FLAG_AUTOGAMESPEED;
}

// FUNCTION: LEGORR 0x004193e0
int info_get_type(const char* name, int* type)
{
    unsigned int i;

    for (i = 0; i < INFO_TYPE_COUNT; i++) {
        if (_strcmpi(infoGlobs.infoName[i], name) == 0) {
            *type = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00419420
Image* info_get_image(int type)
{
    return infoGlobs.infoDataTable[type].image;
}

// FUNCTION: LEGORR 0x00419430
void info_set_overflow_image_file(const char* filename)
{
    infoGlobs.overflowImage = image_load_bmp_scaled(filename, 0, 0);
    resource_record_add_ref(infoGlobs.overflowImage);
}

// FUNCTION: LEGORR 0x00419450
void info_set_auto_game_speed(int on)
{
    if (on) infoGlobs.flags |= INFO_FLAG_AUTOGAMESPEED;
    else infoGlobs.flags &= ~INFO_FLAG_AUTOGAMESPEED;
}

// FUNCTION: LEGORR 0x00419470
void info_set_type_auto_game_speed(int type, int on)
{
    if (on) infoGlobs.infoDataTable[type].flags |= INFO_DATA_FLAG_AUTOGAMESPEED;
    else infoGlobs.infoDataTable[type].flags &= ~INFO_DATA_FLAG_AUTOGAMESPEED;
}

// FUNCTION: LEGORR 0x004194b0
void info_set_text(int type, char* text)
{
    if (infoGlobs.infoDataTable[type].text) free(infoGlobs.infoDataTable[type].text);
    info_compile_text(text, &infoGlobs.infoDataTable[type].text);
}

// FUNCTION: LEGORR 0x004194f0
void info_set_type_play_always(int type, int on)
{
    if (on) infoGlobs.infoDataTable[type].flags |= INFO_DATA_FLAG_PLAYALWAYS;
    else infoGlobs.infoDataTable[type].flags &= ~INFO_DATA_FLAG_PLAYALWAYS;
}

/* Copies the text, turning '_' into a space and "\n" into a newline. A
 * $name$ token, a pointer or an information type, becomes "%b" and its
 * image goes to the trailer after the terminator: the count, then five
 * slots. */
// FUNCTION: LEGORR 0x00419530
void info_compile_text(char* text, char** outText)
{
    int pointerType;
    unsigned int count;
    int type;
    char images[8];
    char name[32];
    char* d;
    char* p;
    int image;
    unsigned int i;

    count = 0;
    d = *outText = (char*)malloc(strlen(text) + 12);

    while (*text != '\0') {
        if (*text == '_') {
            *d = ' ';
        } else if (*text == '$') {
            text++;
            i = 0;
            image = 0;
            p = text;
            while (*p != '$') {
                name[i++] = *p++;
                text++;
            }
            name[i] = '\0';
            if (pointer_cursor_config_parse_target(name, &pointerType)) {
                image = (int)indexed_global_value_at_get(pointerType);
            } else if (info_get_type(name, &type)) {
                image = (int)(uintptr_t)info_get_image(type);
            }
            if (image) {
                *d++ = '%';
                images[count++] = (char)image;
                *d = 'b';
            }
        } else if (*text == '\\' && text[1] == 'n') {
            *d = '\n';
            text++;
        } else {
            *d = *text;
        }
        text++;
        d++;
    }

    *d++ = '\0';
    *d = (char)count;
    for (i = 0; i < 5; i++) {
        d++;
        if (i < count) *d = images[i];
        else *d = '\0';
    }
}

// FUNCTION: LEGORR 0x00419650
void info_set_image_file(int type, const char* filename)
{
    infoGlobs.infoDataTable[type].image = image_load_bmp_scaled(filename, 0, 0);
    resource_record_add_ref(infoGlobs.infoDataTable[type].image);
}

// FUNCTION: LEGORR 0x00419680
void info_set_sfx(int type, int sfxType)
{
    infoGlobs.infoDataTable[type].sfxType = sfxType;
}

// FUNCTION: LEGORR 0x004196a0
int info_message_enumerate(unsigned int index, InfoEnumCallback callback, void* data)
{
    InfoMessageInstance* instance;

    for (instance = infoGlobs.infoMessageTable[index].instanceList; instance; instance = instance->next) {
        if (callback(instance, data)) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004196f0
void info_message_add_instance(unsigned int index, InfoMessageInstance* instance)
{
    InfoMessageInstance* last;
    int count;

    if ((last = infoGlobs.infoMessageTable[index].instanceList)) {
        count = 1;
        while (last->next) {
            if (++count == 9) info_message_remove_instance(index, 0);
            last = last->next;
        }
        last->next = instance;
        instance->next = NULL;
        infoGlobs.infoMessageTable[index].instanceCount++;
    } else {
        infoGlobs.infoMessageTable[index].instanceList = instance;
        instance->next = NULL;
        infoGlobs.infoMessageTable[index].instanceCount++;
    }
}

// FUNCTION: LEGORR 0x00419780
InfoMessageInstance* info_message_get_instance(unsigned int index, int instIndex)
{
    InfoMessageInstance* instance = infoGlobs.infoMessageTable[index].instanceList;
    int i;

    if (instIndex >= 0 && instIndex < (int)infoGlobs.infoMessageTable[index].instanceCount) {
        for (i = 0; i < instIndex; i++) instance = instance->next;
        return instance;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004197b0
InfoMessageInstance* info_message_remove_instance(unsigned int index, int instIndex)
{
    InfoMessageInstance* instance;
    InfoMessageInstance* prev;

    if (!(instance = info_message_get_instance(index, instIndex))) return NULL;
    prev = info_message_get_instance(index, instIndex - 1);
    if (instIndex == 0) infoGlobs.infoMessageTable[index].instanceList = instance->next;
    else prev->next = instance->next;
    infoGlobs.infoMessageTable[index].instanceCount--;
    return instance;
}

// FUNCTION: LEGORR 0x00419810
static int info_callback_find_object(InfoMessageInstance* instance, void* data)
{
    InfoSearchObject* search = (InfoSearchObject*)data;

    if (instance->object == search->object) return 1;
    search->index++;
    return 0;
}

// FUNCTION: LEGORR 0x00419830
void info_remove_object_references(void* object)
{
    InfoSearchObject search;
    unsigned int i;

    search.object = object;
    for (i = 0; i < infoGlobs.infoMessageCount; i++) {
        search.index = 0;
        while (info_message_enumerate(i, info_callback_find_object, &search)) {
            info_message_remove_instance(i, search.index);
            search.index = 0;
        }
        if (infoGlobs.infoMessageTable[i].instanceCount <= 0) {
            pending_record_list_entry_remove_and_release(i--);
        }
    }
}

// FUNCTION: LEGORR 0x004198c0
static int info_callback_find_block(InfoMessageInstance* instance, void* data)
{
    InfoSearchBlock* search = (InfoSearchBlock*)data;

    if (instance->blockPos.x == search->blockPos->x && instance->blockPos.y == search->blockPos->y) return 1;
    search->index++;
    return 0;
}

// FUNCTION: LEGORR 0x004198f0
void info_remove_block_references(InfoPoint2I* blockPos)
{
    InfoSearchBlock search;
    unsigned int i;

    search.blockPos = blockPos;
    for (i = 0; i < infoGlobs.infoMessageCount; i++) {
        if (infoGlobs.infoMessageTable[i].type == INFO_TYPE_LANDSLIDE ||
            infoGlobs.infoMessageTable[i].type == INFO_TYPE_CAVEIN) {
            search.index = 0;
            while (info_message_enumerate(i, info_callback_find_block, &search)) {
                info_message_remove_instance(i, search.index);
                search.index = 0;
            }
            if (infoGlobs.infoMessageTable[i].instanceCount <= 0) {
                pending_record_list_entry_remove_and_release(i--);
            }
        }
    }
}

// FUNCTION: LEGORR 0x004199a0
static int info_callback_find_duplicate(InfoMessageInstance* instance, void* data)
{
    InfoSearchDuplicate* search = (InfoSearchDuplicate*)data;

    if ((search->object == NULL && search->blockPos == NULL) ||
        (search->object && instance->object == search->object) ||
        (search->blockPos && instance->blockPos.x == search->blockPos->x &&
         instance->blockPos.y == search->blockPos->y)) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004199f0
int info_is_duplicate(int type, void* object, InfoPoint2I* blockPos)
{
    InfoSearchDuplicate search;
    unsigned int i;

    if (type != INFO_TYPE_LEGOMANDEATH && type != INFO_TYPE_BUILDINGDEATH &&
        type != INFO_TYPE_VEHICLEDEATH && type != INFO_TYPE_GENERICDEATH) {
        search.blockPos = blockPos;
        search.object = object;
        for (i = 0; i < infoGlobs.infoMessageCount; i++) {
            if (type == infoGlobs.infoMessageTable[i].type) {
                if (info_message_enumerate(i, info_callback_find_duplicate, &search)) return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00419a80
InfoMessageInstance* info_create_instance(InfoPoint2I* blockPos, void* object, char* text)
{
    InfoMessageInstance* instance;

    if (!(instance = (InfoMessageInstance*)malloc(sizeof(InfoMessageInstance)))) return NULL;
    if (blockPos) instance->blockPos = *blockPos;
    else instance->blockPos.x = instance->blockPos.y = -1;
    instance->object = object;
    if (text) info_compile_text(text, &instance->text);
    else instance->text = NULL;
    return instance;
}

// FUNCTION: LEGORR 0x00419ae0
void info_enable(int on)
{
    if (on) infoGlobs.flags |= INFO_FLAG_ENABLED;
    else infoGlobs.flags &= ~INFO_FLAG_ENABLED;
}

// FUNCTION: LEGORR 0x00419b00
int info_has_text(int type)
{
    return infoGlobs.infoDataTable[type].text != NULL;
}

// FUNCTION: LEGORR 0x00419b20
int info_find_message(int type)
{
    int i;

    for (i = 0; i < (int)infoGlobs.infoMessageCount; i++) {
        if (infoGlobs.infoMessageTable[i].type == type) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x00419b50
float info_get_next_y(void)
{
    float y = 333.0f;
    unsigned int i;

    for (i = 0; i < infoGlobs.infoMessageCount; i++) {
        y -= (int)infoGlobs.infoDataTable[infoGlobs.infoMessageTable[i].type].image->height;
    }
    return y;
}

// FUNCTION: LEGORR 0x00419b80
void info_send(int type, char* text, void* object, InfoPoint2I* blockPos)
{
    Image* image;
    int index;

    if (infoGlobs.flags & INFO_FLAG_ENABLED) {
        if (!(system_state_flags_get() & 0x100)) {
            if ((infoGlobs.flags & INFO_FLAG_SOUNDS) &&
                (infoGlobs.infoDataTable[type].flags & INFO_DATA_FLAG_PLAYALWAYS) &&
                infoGlobs.infoDataTable[type].timer <= 0.0f) {
                infoGlobs.infoDataTable[type].timer = 25.0f;
                if (type == INFO_TYPE_LEGOMANDEATH || type == INFO_TYPE_VEHICLEDEATH ||
                    type == INFO_TYPE_BUILDINGDEATH) {
                    infoGlobs.infoDataTable[INFO_TYPE_LEGOMANDEATH].timer = 25.0f;
                    infoGlobs.infoDataTable[INFO_TYPE_VEHICLEDEATH].timer = 25.0f;
                    infoGlobs.infoDataTable[INFO_TYPE_BUILDINGDEATH].timer = 25.0f;
                    infoGlobs.infoDataTable[INFO_TYPE_GENERICDEATH].timer = 25.0f;
                }
                sound_submit_with_global_cooldown(infoGlobs.infoDataTable[type].sfxType, NULL);
            }

            if (!info_is_duplicate(type, object, blockPos)) {
                if (info_has_text(type)) {
                    image = infoGlobs.infoDataTable[type].image;
                    if ((infoGlobs.flags & INFO_FLAG_SOUNDS) &&
                        !(infoGlobs.infoDataTable[type].flags & INFO_DATA_FLAG_PLAYALWAYS) &&
                        infoGlobs.infoDataTable[type].timer <= 0.0f) {
                        infoGlobs.infoDataTable[type].timer = 25.0f;
                        if (type == INFO_TYPE_LEGOMANDEATH || type == INFO_TYPE_VEHICLEDEATH ||
                            type == INFO_TYPE_BUILDINGDEATH || type == INFO_TYPE_GENERICDEATH) {
                            infoGlobs.infoDataTable[INFO_TYPE_LEGOMANDEATH].timer = 25.0f;
                            infoGlobs.infoDataTable[INFO_TYPE_VEHICLEDEATH].timer = 25.0f;
                            infoGlobs.infoDataTable[INFO_TYPE_BUILDINGDEATH].timer = 25.0f;
                            infoGlobs.infoDataTable[INFO_TYPE_GENERICDEATH].timer = 25.0f;
                        }
                        sound_submit_with_global_cooldown(infoGlobs.infoDataTable[type].sfxType, NULL);
                    }

                    if (infoGlobs.infoMessageCount != INFO_MESSAGE_MAX) {
                        if ((index = info_find_message(type)) == -1) {
                            index = infoGlobs.infoMessageCount++;
                            infoGlobs.infoMessageTable[index].x = -(float)(int)image->width;
                            infoGlobs.infoMessageTable[index].y = info_get_next_y();
                        }
                        infoGlobs.infoMessageTable[index].type = type;
                        info_message_add_instance(index, info_create_instance(blockPos, object, text));

                        if ((unsigned int)engine_app_mode_get() <= 4 && (infoGlobs.flags & INFO_FLAG_AUTOGAMESPEED) &&
                            (infoGlobs.infoDataTable[type].flags & INFO_DATA_FLAG_AUTOGAMESPEED)) {
                            if (EngineTimeScale() > 1.0f) scroll_scale_set_and_clamp(1.0f);
                        }
                    }
                }
            }
        }
    }
}

/* Info_GotoFirst: move the view to the first message's object or block. */
// FUNCTION: LEGORR 0x00419da0
void incident_message_panel_refresh_if_active(void)
{
    InfoMessageInstance* instance;

    if (infoGlobs.infoMessageCount > 0 && infoGlobs.infoMessageTable[0].instanceCount) {
        instance = infoGlobs.infoMessageTable[0].instanceList;
        if (instance->object || (instance->blockPos.x > 0 && instance->blockPos.y > 0)) {
            lego_goto_info(instance->object, (const int*)&instance->blockPos, 0);
        }
    }
}

/* Brings message type `index` to the front; the icon positions stay. */
// FUNCTION: LEGORR 0x00419de0
void pending_record_list_entry_move_to_front(unsigned int index)
{
    InfoMessage temp;
    float x, y;
    unsigned int i;

    if (index < infoGlobs.infoMessageCount) {
        if (index > 0) {
            x = infoGlobs.infoMessageTable[0].x;
            y = infoGlobs.infoMessageTable[0].y;
            temp = infoGlobs.infoMessageTable[index];
            for (i = index; i > 0; i--) infoGlobs.infoMessageTable[i] = infoGlobs.infoMessageTable[i - 1];
            infoGlobs.infoMessageTable[0] = temp;
            infoGlobs.infoMessageTable[0].x = x;
            infoGlobs.infoMessageTable[0].y = y;
        }
        infoGlobs.flags |= INFO_FLAG_DOCKED | INFO_FLAG_TEXTCHANGED;
    }
}

// FUNCTION: LEGORR 0x00419e60
void pending_record_release_if_present(void)
{
    if (infoGlobs.infoMessageCount > 0) pending_record_list_entry_remove_and_release(0);
}

/* Info_RemoveMessage: drops the first instance; the type goes when empty. */
// FUNCTION: LEGORR 0x00419e80
void pending_record_list_entry_remove_and_release(unsigned int index)
{
    InfoMessageInstance* instance;
    unsigned int i;

    if (index < infoGlobs.infoMessageCount) {
        if ((instance = info_message_remove_instance(index, 0)) && instance->text) free(instance->text);
        if (infoGlobs.infoMessageTable[index].instanceCount == 0) {
            for (i = index; i < infoGlobs.infoMessageCount; i++) {
                infoGlobs.infoMessageTable[i] = infoGlobs.infoMessageTable[i + 1];
            }
            infoGlobs.infoMessageCount--;
        }
        if (index == 0) infoGlobs.flags &= ~INFO_FLAG_DOCKED;
    }
}

// FUNCTION: LEGORR 0x00419f10
void pending_record_release_all(void)
{
    while (infoGlobs.infoMessageCount) pending_record_list_entry_remove_and_release(0);
}

// FUNCTION: LEGORR 0x00419f30
void info_draw(float elapsed)
{
    Point2F pos;
    unsigned int i;
    Image* image;

    pos.x = 0.0f;
    pos.y = 333.0f;
    for (i = 0; i < infoGlobs.infoMessageCount; i++) {
        image = infoGlobs.infoDataTable[infoGlobs.infoMessageTable[i].type].image;
        if (infoGlobs.infoMessageTable[i].y < pos.y) infoGlobs.infoMessageTable[i].y -= elapsed * -8.0f;
        if (infoGlobs.infoMessageTable[i].y > pos.y) infoGlobs.infoMessageTable[i].y = pos.y;
        if (infoGlobs.infoMessageTable[i].x < 0.0f) infoGlobs.infoMessageTable[i].x -= elapsed * -3.0f;
        if (infoGlobs.infoMessageTable[i].x > 0.0f) infoGlobs.infoMessageTable[i].x = 0.0f;
        pos.y = infoGlobs.infoMessageTable[i].y;
        pos.x = infoGlobs.infoMessageTable[i].x;
        image_display_scaled(image, NULL, &pos, NULL);
        if (pos.y < 0.0f) {
            pos.y = 0.0f;
            image_display_scaled(infoGlobs.overflowImage, NULL, &pos, NULL);
            return;
        }
        font_print_f(info_get_font(),
                     (int)pos.x + 2, (int)pos.y + 10,
                     // STRING: LEGORR 0x004a2b54
                     "%i", infoGlobs.infoMessageTable[i].instanceCount);
        pos.y -= (int)image->height;
    }
}

// FUNCTION: LEGORR 0x0041a080
void info_draw_panel(float elapsed)
{
    unsigned int len;
    unsigned char* text;

    if ((infoGlobs.flags & INFO_FLAG_DOCKED) && infoGlobs.infoMessageCount > 0) {
        if (!(panelGlobs.panelTable[INFO_PANEL].flags & PANEL_FLAG_OUT)) panel_state_flags_toggle(INFO_PANEL);
        if (infoGlobs.flags & INFO_FLAG_TEXTCHANGED) {
            ui_text_output_object_release_target((void* const*)panelGlobs.scrollTextWindow);
            if (!(text = (unsigned char*)infoGlobs.infoMessageTable[0].instanceList->text)) {
                text = (unsigned char*)infoGlobs.infoDataTable[infoGlobs.infoMessageTable[0].type].text;
            }
            len = strlen((char*)text);
            if (text[len + 1] > 0) {
                panel_text_window_print_f(panelGlobs.scrollTextWindow, (char*)text, text[len + 2],
                                          text[len + 3], text[len + 4], text[len + 5], text[len + 6]);
            } else {
                panel_text_window_print_f(panelGlobs.scrollTextWindow, (char*)text);
            }
            ui_text_output_position_and_draw(panelGlobs.scrollTextWindow, infoGlobs.scrollOffset, elapsed);
            scroll_offset_from_cursor_update();
            infoGlobs.flags &= ~INFO_FLAG_TEXTCHANGED;
        }
    } else {
        if (!(panelGlobs.panelTable[INFO_PANEL].flags & PANEL_FLAG_IN)) panel_state_flags_toggle(INFO_PANEL);
    }
    ui_text_output_position_and_draw(panelGlobs.scrollTextWindow, infoGlobs.scrollOffset, elapsed);
}

/* Info_CheckMouse: the mouse over a message icon; a click docks the first
 * message or brings another to the front. */
// FUNCTION: LEGORR 0x0041a1a0
int ui_entry_hit_test_and_dispatch(int mouseX, int mouseY, int leftButton)
{
    unsigned int i;
    Image* image;
    int y;
    int height, width;

    for (i = 0; i < infoGlobs.infoMessageCount; i++) {
        image = infoGlobs.infoDataTable[infoGlobs.infoMessageTable[i].type].image;
        width = image->width;
        height = image->height;
        if (mouseX >= 0 && mouseX <= width) {
            y = (int)infoGlobs.infoMessageTable[i].y;
            if (mouseY >= y && mouseY <= height + y) {
                if (leftButton) {
                    if ((infoGlobs.flags & INFO_FLAG_DOCKED) && i == 0) {
                        panel_state_flags_toggle(INFO_PANEL);
                        infoGlobs.flags &= ~INFO_FLAG_DOCKED;
                        return 1;
                    }
                    pending_record_list_entry_move_to_front(i);
                }
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041a250
void scroll_offset_from_cursor_update(void)
{
    int lines;
    int linesShown;
    int offset;

    ui_record_callback_forward((const void* const*)panelGlobs.scrollTextWindow,
                               (uint32_t*)&lines, (uint32_t*)&linesShown);
    offset = lines - linesShown;
    if (offset < 0) offset = 0;
    indexed_text_metrics_pair_set(0, offset, 0);
    screen_right_edge_cache_refresh();
}

// FUNCTION: LEGORR 0x0041a290
void screen_right_edge_cache_refresh(void)
{
    int range;
    int position;

    indexed_text_metrics_pair_get(0, (uint32_t*)&range, (uint32_t*)&position);
    infoGlobs.scrollOffset = range - position;
}

// FUNCTION: LEGORR 0x0041a2c0
void info_update_timers(float elapsed)
{
    unsigned int i;

    for (i = 0; i < INFO_TYPE_COUNT; i++) {
        if (infoGlobs.infoDataTable[i].timer > 0.0f) infoGlobs.infoDataTable[i].timer -= elapsed;
    }
}

// FUNCTION: LEGORR 0x0041a2f0
struct TextureSurface* info_get_font(void)
{
    return interfaceGlobs.font;
}

// PORT-ONLY: the port tail and the tests read the scroll offset
int info_scroll_offset_get(void)
{
    return infoGlobs.scrollOffset;
}
