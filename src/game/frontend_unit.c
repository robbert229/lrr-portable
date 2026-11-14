// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-017: the front end (the original's FrontEnd.c), rebuilt in original
 * shape (ADR 0005, finding 623). Text 004102b0-00418190; this is part 1,
 * 004102b0-00414de0. Part 2 (00414df0-00418190) is still the relocated
 * port code in frontend_port.cpp, which also keeps the port's models of
 * part 1, unannotated. See frontend_unit.h for the records. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* savegame.h: this unit defines its part-2 names (finding 628). */
#define LEGORRR_FRONTEND_UNIT_C 1
#include "frontend_unit_link_names.h"
#include "frontend_unit.h"
#include "frontend_unit_imports.h"
#include "crt/vc5names.h"
#include "gods98/winmm.h"

/* Input.c's mouse is signed; the port's words are unsigned. */
#define FRONT_MSX ((int)s_renderer_word_a)
#define FRONT_MSY ((int)s_renderer_word_b)

/* Config_GetIntValue: the string, or "" without one. */
#define FRONT_CONFIG_INT(path) \
    atoi(block_get_strptr(legoGlobs_config, (path)) \
             ? block_get_strptr(legoGlobs_config, (path)) \
             : s_config_angle_empty)

#define FRONT_PRINT_MAX 15

/* mainGlobs.programmerLevel, compared unsigned. */
#define FRONT_PROGRAMMER_LEVEL ((unsigned int)s_application_mode)

/* frontGlobs.rockWipeFlags. */
#define FRONT_ROCKWIPE_ANIMATING 0x1
#define FRONT_ROCKWIPE_NOINPUT 0x2

/* The menu text window record, 0x728 bytes (00415d00 loads one from a
 * Menu::SaveText or Menu::LevelText block): the save menu's (frontGlobs
 * +0x5d4) and the level select's (+0x5d8), two views of one layout. */
typedef struct Front_SaveTextWindow {
    void* textWindow;               /* +0x000 */
    int field_4[9];                 /* +0x004 */
    char loadText[256];             /* +0x028 "Load" */
    char saveText[256];             /* +0x128 "Save" */
    char slotText[256];             /* +0x228 "Slot" */
    char slotNumberText[256];       /* +0x328 "LoadSel" */
} Front_SaveTextWindow;

typedef struct Front_LevelTextWindow {
    void* textWindow;               /* +0x000 */
    struct Image* panelImage;       /* +0x004 */
    float window[4];                /* +0x008 "Window" x, y, w, h */
    float panelPos[2];              /* +0x018 "Panel" */
    float panelSize[2];             /* +0x020 */
    char loadText[256];             /* +0x028 "Load" */
    char saveText[256];             /* +0x128 "Save" */
    char slotText[256];             /* +0x228 "Slot" */
    char loadSelText[256];          /* +0x328 "LoadSel" */
    char saveSelText[256];          /* +0x428 "SaveSel" */
    char levelText[256];            /* +0x528 "Level" */
    char tutorialText[256];         /* +0x628 "Tutorial" */
} Front_LevelTextWindow;

/* The original's FrontEnd.c .bss (004d78c8-004db8e8). */
// GLOBAL: LEGORR 0x004d78c8
static int s_printXList[FRONT_PRINT_MAX];
// GLOBAL: LEGORR 0x004d7b08
static Front_Font* s_printFontList[FRONT_PRINT_MAX];
// GLOBAL: LEGORR 0x004d7b48
static char s_printTextList[FRONT_PRINT_MAX][1024];
// GLOBAL: LEGORR 0x004db748
static char s_replaceSpacesBuffer[256];
// GLOBAL: LEGORR 0x004db850
static int s_printYList[FRONT_PRINT_MAX];
// GLOBAL: LEGORR 0x004db890
static struct Image* s_saveSlotImages[5];
// GLOBAL: LEGORR 0x004db8a8
static int s_saveSlotCompletion[5];
// GLOBAL: LEGORR 0x004db8bc
static int s_saveMenuOverwriteShowing;
// GLOBAL: LEGORR 0x004db8c0
static int s_overlayTriggered;
// GLOBAL: LEGORR 0x004db8c4
static int s_loadSaveTextShown;
// GLOBAL: LEGORR 0x004db8c8
static Front_Menu* s_savedNextMenu;
// GLOBAL: LEGORR 0x004db8cc
static Front_Cache* s_cacheList;
// GLOBAL: LEGORR 0x004db8d0
static unsigned int s_printCount;
// GLOBAL: LEGORR 0x004db8d4
static int s_mouseLeftPrevious;
// GLOBAL: LEGORR 0x004db8dc
static int s_textInputBlink;

/* FrontEnd.c's initialised .data (004a1f48-004a1f60). */
// GLOBAL: LEGORR 0x004a1f48
static int s_selectHoverIndex = -1;
// GLOBAL: LEGORR 0x004a1f4c
static int s_selectClickIndex = -1;
// GLOBAL: LEGORR 0x004a1f50
static int s_overlayIndex = -1;
// GLOBAL: LEGORR 0x004a1f54
static int s_overlayShownIndex = -1;
// GLOBAL: LEGORR 0x004a1f58
static int s_overwriteAnswer = -1;
// GLOBAL: LEGORR 0x004a1f5c
static int s_saveSlotToWrite = -1;
/* Input_GetKeyCharacter's table: each DirectInput key's character. */
// GLOBAL: LEGORR 0x004a1f60
static const unsigned int s_keyCharacters[256] = {
    0x00, 0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x30, 0x00, 0x00, 0x00, 0x00,
    0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69,
    0x6f, 0x70, 0x00, 0x00, 0x00, 0x00, 0x61, 0x73,
    0x64, 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7a, 0x78, 0x63, 0x76,
    0x62, 0x6e, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37,
    0x38, 0x39, 0x00, 0x34, 0x35, 0x36, 0x00, 0x31,
    0x32, 0x33, 0x30, 0x00
};

/* frontGlobs (00556fc0), in the common region. Fields the port does not
 * model are stand-ins, one annotated global per field; the reward screen
 * (reward.c) reads three of them by their frontend_api.h names. */
// GLOBAL: LEGORR 0x00556fc4
void* g_front_main_menu_set;
// GLOBAL: LEGORR 0x00556fcc
Front_MenuSet* frontGlobs_saveMenuSet;
// GLOBAL: LEGORR 0x00557028
int frontGlobs_f_557028;
// GLOBAL: LEGORR 0x00557064
int frontGlobs_overlayY;
// GLOBAL: LEGORR 0x00557068
int frontGlobs_overlayX;
// GLOBAL: LEGORR 0x0055706c
struct Flic* frontGlobs_overlayImage;
// GLOBAL: LEGORR 0x00557070
unsigned int frontGlobs_overlayStartTime;
// GLOBAL: LEGORR 0x00557074
unsigned int frontGlobs_overlayCurrTime;
// GLOBAL: LEGORR 0x00557078
Front_Font* frontGlobs_versionFont;
// GLOBAL: LEGORR 0x0055707c
char* frontGlobs_versionString;
// GLOBAL: LEGORR 0x005574dc
int frontGlobs_scrollOffsetX;
// GLOBAL: LEGORR 0x005574e0
int frontGlobs_scrollOffsetY;
// GLOBAL: LEGORR 0x005574ec
void* frontGlobs_rockWipeAnim;
// GLOBAL: LEGORR 0x005574f0
unsigned int frontGlobs_rockWipeFlags;
// GLOBAL: LEGORR 0x005574f4
float frontGlobs_rockWipeTimer;
// GLOBAL: LEGORR 0x005574f8
float frontGlobs_rockWipeLastUpdateTime;
// GLOBAL: LEGORR 0x005574fc
void* frontGlobs_rockWipeLight;
// GLOBAL: LEGORR 0x00557504
int g_front_save_menu_result;
// GLOBAL: LEGORR 0x00557508
int g_front_save_menu_open;
/* Unsigned: 004124f0 converts them with a zero high word (fild qword). */
// GLOBAL: LEGORR 0x0055750c
unsigned int frontGlobs_saveImageBigWidth;
// GLOBAL: LEGORR 0x00557510
unsigned int frontGlobs_saveImageBigHeight;
// GLOBAL: LEGORR 0x00557514
char frontGlobs_saveSlotFormat[64];
// GLOBAL: LEGORR 0x00557594
Front_SaveTextWindow* frontGlobs_saveTextWnd;
// GLOBAL: LEGORR 0x00557598
Front_LevelTextWindow* frontGlobs_saveLevelWnd;
// GLOBAL: LEGORR 0x0055759c
char frontGlobs_overwriteTitle[128];
// GLOBAL: LEGORR 0x0055761c
char frontGlobs_overwriteText[256];
// GLOBAL: LEGORR 0x0055771c
char frontGlobs_overwriteOK[128];
// GLOBAL: LEGORR 0x0055779c
char frontGlobs_overwriteCancel[128];

#define frontGlobs_mainMenuSet ((Front_MenuSet*)g_front_main_menu_set)

/* Front_Menu_CreateOverlay's "Null" (004a23f4), as a named array that
 * 00413f90 and the menu loader compare against. */
// GLOBAL: LEGORR 0x004a23f4
static const char s_overlayNullName[5] = "Null";

/* Callees whose port declarations type a float argument as its bits
 * (view.hpp: 00473460, 00473480) or name it unused (savegame.hpp:
 * 004150b0, which the original passes the frame time). The matching
 * build calls them with the original's float; natively the bits are
 * passed through the port's prototype. */
#ifdef __cplusplus
extern "C" {
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
int light_interface_call_slot_40(void* record, float value);      /* 00473460 */
int light_interface_call_slot_3c(void* record, float value);      /* 00473480 */
void save_menu_refresh_target(float elapsed, int slot);           /* 004150b0 */
#define FRONT_FLOAT_ARG(f) (f)
#else
int light_interface_call_slot_40(void* record, uint32_t value);
int light_interface_call_slot_3c(void* record, uint32_t value);
void save_menu_refresh_target(uint32_t unused, int slot);
#endif
#ifdef __cplusplus
}
#endif
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: native build only; passes a float as the port's uint32_t bits
static uint32_t front_float_bits(float f)
{
    uint32_t bits;

    memcpy(&bits, &f, sizeof(bits));
    return bits;
}
#define FRONT_FLOAT_ARG(f) front_float_bits(f)
#endif

// FUNCTION: LEGORR 0x004102b0
char* front_util_replace_text_spaces(const char* str)
{
    unsigned int i;

    if (strlen(str) >= sizeof(s_replaceSpacesBuffer)) {
        return (char*)str;
    }
    strcpy(s_replaceSpacesBuffer, str);
    i = 0;
    do {
        if (s_replaceSpacesBuffer[i] == '_') {
            s_replaceSpacesBuffer[i] = ' ';
        }
        i++;
    } while (i < sizeof(s_replaceSpacesBuffer));
    return s_replaceSpacesBuffer;
}

// FUNCTION: LEGORR 0x00410320
void front_util_print_f(Front_Font* font, int x, int y, const char* msg, ...)
{
    unsigned int i;
    va_list args;

    if (msg == NULL) {
        for (i = 0; i < s_printCount; i++) {
            font_print_f(s_printFontList[i], s_printXList[i], s_printYList[i],
                         s_printTextList[i]);
        }
        s_printCount = 0;
        return;
    }
    if (s_printCount < FRONT_PRINT_MAX) {
        va_start(args, msg);
        vsprintf(s_printTextList[s_printCount], msg, args);
        s_printXList[s_printCount] = x;
        s_printYList[s_printCount] = y;
        s_printFontList[s_printCount] = font;
        s_printCount++;
        va_end(args);
    }
}

// FUNCTION: LEGORR 0x004103d0
void front_callback_trigger_play_credits(void)
{
#if defined(_MSC_VER) && _MSC_VER < 1200
    credits_play((const char*)DAT_00556f50, (struct TextureSurface*)DAT_0055613c,
                 (const char*)DAT_00556f54);
#else
    /* Natively the port's entry plays the credits (frontend_port.cpp). */
    main_menu_text_item();
#endif
}

// FUNCTION: LEGORR 0x004103f0
#if defined(_MSC_VER) && defined(_M_IX86)
/* No C form matched (finding 623): MSVC folds the two flag ORs into one
 * (or al, 3); the original keeps or al, 1 and or al, 2. Tried: the two
 * |= statements in every order around the timer store, one combined
 * expression, and an enum flag type. With frontGlobs as one struct the
 * C version below compiles to the original's code (finding 635), so this
 * waits for that layout. Naked asm, as the port had it; the C version is
 * the native one. */
__declspec(naked) void front_rock_wipe_play(void)
{
    __asm {
        mov eax, frontGlobs_rockWipeAnim
        sub esp, 8
        test eax, eax
        je rock_wipe_play_done
        mov eax, frontGlobs_rockWipeFlags
        mov dword ptr frontGlobs_rockWipeTimer, 0
        or al, 1
        or al, 2
        mov frontGlobs_rockWipeFlags, eax
        call main_time_get
        mov dword ptr [esp], eax
        mov dword ptr [esp + 4], 0
        fild qword ptr [esp]
        push 0
        push 0x27
        fstp dword ptr frontGlobs_rockWipeLastUpdateTime
        call sfx_add_to_queue
        add esp, 8
    rock_wipe_play_done:
        add esp, 8
        ret
    }
}
#else
void front_rock_wipe_play(void)
{
    if (frontGlobs_rockWipeAnim != NULL) {
        frontGlobs_rockWipeTimer = 0.0f;
        frontGlobs_rockWipeFlags |= FRONT_ROCKWIPE_ANIMATING;
        frontGlobs_rockWipeFlags |= FRONT_ROCKWIPE_NOINPUT;
        frontGlobs_rockWipeLastUpdateTime = (float)main_time_get();
        sfx_add_to_queue(0x27, 0);
    }
}
#endif

// FUNCTION: LEGORR 0x00410440
void front_rock_wipe_stop(void)
{
    frontGlobs_rockWipeFlags &= ~(FRONT_ROCKWIPE_ANIMATING | FRONT_ROCKWIPE_NOINPUT);
}

// FUNCTION: LEGORR 0x00410450
Front_Cache* front_cache_find_by_name(const char* name)
{
    Front_Cache* cache;

    for (cache = s_cacheList; cache != NULL; cache = cache->next) {
        if (_strcmpi(name, cache->path) == 0) {
            return cache;
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00410490
Front_Cache* front_cache_create(const char* name)
{
    Front_Cache* cache;

    if (name != NULL && *name != '\0') {
        cache = front_cache_find_by_name(name);
        if (cache != NULL) {
            return cache;
        }
            cache = (Front_Cache*)malloc(sizeof(Front_Cache));
            cache->path = (char*)malloc(strlen(name) + 1);
            strcpy(cache->path, name);
            cache->image = NULL;
            cache->next = s_cacheList;
            cache->font = NULL;
            s_cacheList = cache;
        return cache;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00410520
struct Image* front_cache_load_image(const char* name)
{
    Front_Cache* cache;

    if (name != NULL && *name != '\0') {
        cache = front_cache_create(name);
        if (cache->image == NULL) {
            cache->image = image_load_bmp_scaled(name, 0, 0);
        }
        return cache->image;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00410560
Front_Font* front_cache_load_font(const char* name)
{
    Front_Cache* cache;

    if (name != NULL && *name != '\0') {
        cache = front_cache_create(name);
        if (cache->font == NULL) {
            cache->font = (Front_Font*)font_atlas_record_create(name);
        }
        return cache->font;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004105a0
char* front_util_string_copy(const char* str)
{
    char* newStr;

    if (str == NULL) {
        return NULL;
    }
    newStr = (char*)malloc(strlen(str) + 1);
    if (newStr == NULL) {
        return NULL;
    }
    strcpy(newStr, str);
    return newStr;
}

// FUNCTION: LEGORR 0x004105f0
Front_SelectData* front_menu_item_create_select(
    int* valuePtr, const char* string1, const char* string2, int x1,
    int y1, int yStep, int visibleCount, int xString1, int yString1,
    int xString2, int yString2, int field_50,
    Front_SelectCallback callback, Front_Menu* nextMenu)
{
    Front_SelectData* select;

    select = (Front_SelectData*)malloc(sizeof(Front_SelectData));
    memset(select, 0, sizeof(Front_SelectData));
    select->string1 = front_util_string_copy(string1);
    select->string2 = front_util_string_copy(string2);
    select->valuePtr = valuePtr;
    select->x1 = x1;
    select->y1 = y1;
    select->yStep = yStep;
    select->visibleCount = visibleCount;
    select->xString1 = xString1;
    select->yString1 = yString1;
    select->xString2 = xString2;
    select->yString2 = yString2;
    select->field_50 = field_50;
    select->callback = callback;
    select->scrollStart = 0;
    select->nextMenu = nextMenu;
    return select;
}

// FUNCTION: LEGORR 0x00410690
void front_menu_item_free_select(Front_SelectData* select)
{
    int i;
    int j;

    if (select != NULL) {
        if (select->string1 != NULL) {
            free(select->string1);
        }
        if (select->string2 != NULL) {
            free(select->string2);
        }
        if (select->selItemList != NULL) {
            for (i = 0; i < select->selItemCount; i++) {
                if (select->selItemList[i].flags & 1) {
                    free(select->selItemList[i].banner);
                } else if (select->selItemList[i].flags & 2) {
                    for (j = 0; j < 3; j++) {
                        image_surface_release(select->selItemList[i].images[j]);
                    }
                }
            }
            free(select->selItemList);
        }
        free(select);
    }
}

// FUNCTION: LEGORR 0x00410740
void front_menu_item_add_select_item(Front_SelectData* select, char* banner,
                                     int frontEndOpen, Front_Font* font,
                                     int xPos, int yPos, int field_1c)
{
    Front_SelectItem* newList;
    int* newWidths[3];
    int* newHeights[3];
    int i;

    if (select->selItemList != NULL) {
        newList = (Front_SelectItem*)realloc(select->selItemList,
                                             (select->selItemCount + 1) * sizeof(Front_SelectItem));
        for (i = 0; i < 3; i++) {
            newWidths[i] = (int*)realloc(select->widths[i], (select->selItemCount + 1) * sizeof(int));
            newHeights[i] = (int*)realloc(select->heights[i], (select->selItemCount + 1) * sizeof(int));
        }
    } else {
        newList = (Front_SelectItem*)malloc((select->selItemCount + 1) * sizeof(Front_SelectItem));
        for (i = 0; i < 3; i++) {
            newWidths[i] = (int*)malloc((select->selItemCount + 1) * sizeof(int));
            newHeights[i] = (int*)malloc((select->selItemCount + 1) * sizeof(int));
        }
    }


    if (newList != NULL) {
        select->selItemList = newList;
        select->selItemList[select->selItemCount].banner = NULL;
        select->selItemList[select->selItemCount].flags = frontEndOpen ? 4 : 0;
        for (i = 0; i < 3; i++) {
            select->widths[i] = newWidths[i];
            select->heights[i] = newHeights[i];
        }

        // STRING: LEGORR 0x004a2360
        if (banner != NULL && case_insensitive_substring_find(banner, ".bmp") != NULL) {
            char* stringParts[3];
            struct Image* image;

            sound_variant_split(banner, stringParts, ",");
            select->selItemList[select->selItemCount].flags |= 2;
            for (i = 0; i < 3; i++) {
                select->selItemList[select->selItemCount].images[i] =
                    front_cache_load_image(stringParts[i]);
                image = select->selItemList[select->selItemCount].images[i];
                if (image != NULL) {
                    resource_record_add_ref(image);
                }
                select->widths[i][select->selItemCount] = image->width;
                select->heights[i][select->selItemCount] = image->height;
            }
            select->selItemList[select->selItemCount].xPos = xPos;
            select->selItemList[select->selItemCount].yPos = yPos;
            select->selItemList[select->selItemCount].frontEndOpen = field_1c;
            select->selItemCount++;
        } else {
            select->selItemList[select->selItemCount].flags |= 1;
            select->selItemList[select->selItemCount].banner = front_util_string_copy(banner);
            select->selItemList[select->selItemCount].xPos = xPos;
            select->selItemList[select->selItemCount].yPos = yPos;
            select->selItemList[select->selItemCount].frontEndOpen = field_1c;
            select->widths[2][select->selItemCount] = formatted_text_printf_width(font, banner);
            select->widths[1][select->selItemCount] = select->widths[2][select->selItemCount];
            select->widths[0][select->selItemCount] = select->widths[1][select->selItemCount];
            select->heights[2][select->selItemCount] = object_field_be4_dword_get(font);
            select->heights[1][select->selItemCount] = select->heights[2][select->selItemCount];
            select->heights[0][select->selItemCount] = select->heights[1][select->selItemCount];
            select->selItemCount++;
        }
    }
}

// FUNCTION: LEGORR 0x00410a10
Front_CycleData* front_menu_item_create_cycle(int count, int* valuePtr,
                                              int x2, int y2,
                                              Front_IntCallback callback)
{
    Front_CycleData* cycle;

    cycle = (Front_CycleData*)malloc(sizeof(Front_CycleData));
    if (cycle == NULL) {
        return NULL;
    }
    cycle->nameList = (char**)malloc(count * sizeof(char*));
    if (cycle->nameList == NULL) {
        front_menu_item_free_cycle(cycle);
        return NULL;
    }
    memset(cycle->nameList, 0, count * sizeof(char*));
    cycle->valuePtr = valuePtr;
    cycle->nameCapacity = count;
    cycle->nameCount = 0;
    cycle->x2 = x2;
    cycle->y2 = y2;
    cycle->callback = callback;
    return cycle;
}

// FUNCTION: LEGORR 0x00410aa0
void front_menu_item_add_cycle_item(Front_CycleData* cycle, const char* name)
{
    if (cycle != NULL && cycle->nameList != NULL && cycle->nameCount < cycle->nameCapacity) {
        cycle->nameList[cycle->nameCount] = front_util_string_copy(name);
        if (cycle->nameList[cycle->nameCount] != NULL) {
            cycle->nameCount++;
        }
    }
}

// FUNCTION: LEGORR 0x00410ae0
void front_menu_item_free_cycle(Front_CycleData* cycle)
{
    int i;

    if (cycle != NULL) {
        if (cycle->nameList != NULL) {
            for (i = 0; i < cycle->nameCount; i++) {
                if (cycle->nameList[i] != NULL) {
                    free(cycle->nameList[i]);
                }
            }
            free(cycle->nameList);
        }
        free(cycle);
    }
}

// FUNCTION: LEGORR 0x00410b30
unsigned int renderer_word_a_get(void)
{
    return s_renderer_word_a;
}

// FUNCTION: LEGORR 0x00410b40
unsigned int renderer_word_b_get(void)
{
    return s_renderer_word_b;
}

// FUNCTION: LEGORR 0x00410b50
unsigned int renderer_word_c_get(void)
{
    return g_renderer_word_c;
}

// FUNCTION: LEGORR 0x00410b60
Front_TriggerData* front_menu_item_create_trigger(int* valuePtr, int end,
                                                  Front_TriggerCallback callback)
{
    Front_TriggerData* trigger;

    trigger = (Front_TriggerData*)malloc(sizeof(Front_TriggerData));
    if (trigger == NULL) {
        return NULL;
    }
    trigger->valuePtr = valuePtr;
    trigger->end = end;
    trigger->callback = callback;
    return trigger;
}

// FUNCTION: LEGORR 0x00410b90
Front_RealSliderData* front_menu_item_create_real_slider(
    float* valuePtr, float valueMin, float valueMax, float step,
    int x2, int y2, Front_RealCallback callback)
{
    Front_RealSliderData* slider;

    if (valueMax <= valueMin) {
        return NULL;
    }
    slider = (Front_RealSliderData*)malloc(sizeof(Front_RealSliderData));
    if (slider == NULL) {
        return NULL;
    }
    slider->valuePtr = valuePtr;
    slider->valueMin = valueMin;
    slider->valueMax = valueMax;
    slider->step = step;
    slider->x2 = x2;
    slider->y2 = y2;
    slider->callback = callback;
    return slider;
}

// FUNCTION: LEGORR 0x00410bf0
Front_SliderData* front_menu_item_create_slider(
    int* valuePtr, int valueMin, int valueMax, int x2, int y2,
    Front_IntCallback callback, struct Image* barOff,
    struct Image* barOn, struct Image* capLeft, struct Image* capRight,
    struct Image* plusLo, struct Image* minusLo, struct Image* plusHi,
    struct Image* minusHi)
{
    Front_SliderData* slider;

    if (valueMax <= valueMin) {
        return NULL;
    }
    slider = (Front_SliderData*)malloc(sizeof(Front_SliderData));
    if (slider == NULL) {
        return NULL;
    }
    slider->valuePtr = valuePtr;
    slider->x2 = x2;
    slider->y2 = y2;
    slider->callback = callback;
    slider->imageBarOff = barOff;
    slider->imageBarOn = barOn;
    slider->imageCapLeft = capLeft;
    slider->imageCapRight = capRight;
    slider->imagePlusLo = plusLo;
    slider->imageMinusLo = minusLo;
    slider->valueMin = valueMin;
    slider->valueMax = valueMax;
    slider->imagePlusHi = plusHi;
    slider->imageMinusHi = minusHi;
    return slider;
}

// FUNCTION: LEGORR 0x00410c70
void front_menu_item_free_menu_item(Front_MenuItem* item)
{
    if (item != NULL) {
        if (item->banner != NULL) {
            free(item->banner);
        }
        switch (item->itemType) {
        case Front_MenuItem_Cycle:
            front_menu_item_free_cycle((Front_CycleData*)item->itemData);
            break;
        case Front_MenuItem_Trigger:
            optional_resource_destroy(item->itemData);
            break;
        case Front_MenuItem_TextInput:
            optional_resource_destroy(item->itemData);
            break;
        case Front_MenuItem_Slider:
            optional_resource_destroy(item->itemData);
            break;
        case Front_MenuItem_RealSlider:
            optional_resource_destroy(item->itemData);
            break;
        case Front_MenuItem_Select:
            front_menu_item_free_select((Front_SelectData*)item->itemData);
            break;
        }
        free(item);
    }
}

// FUNCTION: LEGORR 0x00410d50
Front_MenuItem* front_menu_item_create_banner_item(
    const char* banner, Front_Font* fontLo, Front_Font* fontHi,
    int x1, int y1, int itemType, int centered, void* itemData,
    int notInTuto)
{
    Front_MenuItem* item;

    if (banner == NULL) {
        return NULL;
    }
    item = (Front_MenuItem*)malloc(sizeof(Front_MenuItem));
    if (item == NULL) {
        return NULL;
    }
    memset(item, 0, sizeof(Front_MenuItem));
    item->length = strlen(banner);
    item->banner = front_util_string_copy(banner);
    item->fontLo = fontLo;
    item->fontHi = fontHi;
    item->itemType = itemType;
    item->itemData = itemData;
    item->x1 = x1;
    item->y1 = y1;
    item->notInTuto = notInTuto;
    if (centered && fontLo != NULL && fontHi != NULL) {
        item->centerOffLo = -(int)(formatted_text_printf_width(fontLo, banner) / 2);
        item->centerOffHi = -(int)(formatted_text_printf_width(fontHi, banner) / 2);
    } else {
        item->centerOffLo = 0;
        item->centerOffHi = 0;
    }
    return item;
}

// FUNCTION: LEGORR 0x00410e20
Front_MenuItem* front_menu_item_create_image_item(
    const char* banner, Front_Font* fontLo, Front_Font* fontHi,
    const char* loImageName, const char* hiImageName, int x1, int y1,
    int itemType, int centered, const char* toolTipName, void* itemData)
{
    Front_MenuItem* item;

    if (banner != NULL) {
        item = (Front_MenuItem*)malloc(sizeof(Front_MenuItem));
        if (item != NULL) {
            memset(item, 0, sizeof(Front_MenuItem));
            item->length = strlen(banner);
            item->banner = front_util_string_copy(banner);
            item->fontLo = fontLo;
            item->fontHi = fontHi;
            item->imageLo = image_load_bmp_scaled(loImageName, 0, 0);
            if (item->imageLo != NULL) {
                item->imageHi = image_load_bmp_scaled(hiImageName, 0, 0);
                if (item->imageHi != NULL) {
                    item->isImageItem = 1;
                }
                item->itemType = itemType;
                item->itemData = itemData;
                item->x1 = x1;
                item->y1 = y1;
                if (!tooltip_get_type(toolTipName, &item->toolTipType)) {
                    item->toolTipType = 0;
                }
                if (centered && item->imageLo != NULL && item->imageHi != NULL) {
                    item->centerOffLo = -(int)(item->imageLo->width / 2);
                    item->centerOffHi = -(int)(item->imageHi->width / 2);
                } else {
                    item->centerOffLo = 0;
                    item->centerOffHi = 0;
                }
                return item;
            }
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00410f30
void front_menu_free_menu(Front_Menu* menu)
{
    int i;

    if (menu != NULL) {
        if (menu->title != NULL) {
            free(menu->title);
        }
        if (menu->fullName != NULL) {
            free(menu->fullName);
        }
        if (menu->items != NULL) {
            free(menu->items);
        }
        for (i = 0; i < menu->itemCount; i++) {
            if (menu->items[i] != NULL) {
                front_menu_item_free_menu_item(menu->items[i]);
            }
        }
        if (menu->menuImage != NULL) {
            image_surface_release(menu->menuImage);
        }
        free(menu);
    }
}

// FUNCTION: LEGORR 0x00410fb0
int front_menu_load_menu_image(Front_Menu* menu, const char* filename, int light)
{
    char* stringParts[4];
    char buff[1024];
    unsigned int numParts;

    stringParts[0] = NULL;
    stringParts[1] = NULL;
    stringParts[2] = NULL;
    stringParts[3] = NULL;
    if (filename == NULL) {
        return 0;
    }
    strcpy(buff, filename);
    numParts = sound_variant_split(buff, stringParts, s_config_rgb_separator);
    if (numParts >= 3) {
        menu->flags |= 1;
        menu->imagePos[0] = (float)atof(stringParts[1]);
        menu->imagePos[1] = (float)atof(stringParts[2]);
    }
    if (light) {
        menu->menuImage = front_cache_load_image(stringParts[0]);
        if (menu->menuImage != NULL && numParts == 4) {
            if (atoi(stringParts[3]) == 1) {
                resource_record_add_ref(menu->menuImage);
            }
        }
        return menu->menuImage != NULL;
    }
    menu->menuImageDark = front_cache_load_image(stringParts[0]);
    if (menu->menuImageDark != NULL && numParts == 4) {
        if (atoi(stringParts[3]) == 1) {
            image_surface_blit_transformed((struct TextureSurface*)menu->menuImageDark,
                                           0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    return menu->menuImageDark != NULL;
}

// FUNCTION: LEGORR 0x00411100
Front_Menu* front_menu_create_menu(const char* title, const char* fullName,
                                   Front_Font* menuFont, int xPos, int yPos,
                                   int autoCenter, int displayTitle,
                                   int centerOffset, int canScroll,
                                   const char* anchored)
{
    Front_Menu* menu;
    char* stringParts[100];

    menu = (Front_Menu*)malloc(sizeof(Front_Menu));
    if (menu == NULL) {
        return NULL;
    }
    memset(menu, 0, sizeof(Front_Menu));
    menu->title = front_util_string_copy(title);
    menu->fullName = front_util_string_copy(fullName);
    if (menu->title == NULL || menu->fullName == NULL) {
        return NULL;
    }
    menu->titleLength = strlen(title);
    menu->items = (Front_MenuItem**)malloc(15 * sizeof(Front_MenuItem*));
    if (menu->items == NULL) {
        front_menu_free_menu(menu);
        return NULL;
    }
    menu->menuFont = menuFont;
    menu->yPos = yPos;
    menu->autoCenter = autoCenter;
    menu->displayTitle = displayTitle;
    menu->xPos = xPos;
    menu->itemCapacity = 15;
    menu->itemCount = 0;
    menu->itemFocus = 0;
    menu->closed = 0;
    menu->centerOffset = centerOffset;
    menu->flags = canScroll ? 2 : 0;
    if (anchored != NULL) {
        menu->anchored = 1;
        sound_variant_split((char*)anchored, stringParts, s_config_rgb_separator);
        menu->anchoredX = atoi(stringParts[0]);
        menu->anchoredY = atoi(stringParts[1]);
    }
    return menu;
}

// FUNCTION: LEGORR 0x00411260
int front_menu_add_menu_item(Front_Menu* menu, Front_MenuItem* item)
{
    Front_MenuItem** newItems;

    if (item == NULL || menu == NULL) {
        return 0;
    }
    if (menu->itemCount == menu->itemCapacity) {
        if (menu->items == NULL) {
            newItems = (Front_MenuItem**)malloc((menu->itemCapacity + 15) * sizeof(Front_MenuItem*));
        } else {
            newItems = (Front_MenuItem**)realloc(menu->items, (menu->itemCapacity + 15) * sizeof(Front_MenuItem*));
        }
        if (newItems == NULL) {
            return 0;
        }
        menu->items = newItems;
        menu->itemCapacity += 15;
    }
    menu->items[menu->itemCount] = item;
    menu->itemCount++;
    return 1;
}

// FUNCTION: LEGORR 0x004112e0
int front_maths_is_point_inside_rect(int ptX, int ptY, int rcX, int rcY,
                                     int rcWidth, int rcHeight)
{
    if (ptX >= rcX && ptX <= rcX + rcWidth) {
        if (ptY >= rcY && ptY <= rcY + rcHeight) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00411320
int front_maths_is_point_inside_rect_centered(int ptX, int ptY, int rcX,
                                              int rcY, int rcWidth,
                                              int rcHeight, int centered)
{
    int offset;

    if (centered) {
        offset = -(rcWidth / 2);
    } else {
        offset = 0;
    }
    return front_maths_is_point_inside_rect(ptX, ptY, rcX + offset, rcY, rcWidth, rcHeight);
}

// FUNCTION: LEGORR 0x00411360
int front_menu_item_select_test_string_collision(Front_Menu* menu,
                                                 Front_MenuItem* item,
                                                 Front_SelectData* select)
{
    int width;
    int height;
    int i;
    int index;
    Front_SelectItem* selItem;

    height = 0;
    if (item->fontHi != NULL) {
        width = formatted_text_printf_width(item->fontHi, select->string1);
        height = object_field_be4_dword_get(item->fontHi);
    } else {
        width = 0;
    }
    if (front_maths_is_point_inside_rect_centered(FRONT_MSX, FRONT_MSY,
                                                  select->xString1 + menu->xPos,
                                                  select->yString1 + menu->yPos,
                                                  width, height, 0)) {
        return -2;
    }
    if (item->fontHi != NULL) {
        width = formatted_text_printf_width(item->fontHi, select->string2);
    } else {
        width = 0;
    }
    if (front_maths_is_point_inside_rect_centered(FRONT_MSX, FRONT_MSY,
                                                  select->xString2 + menu->xPos,
                                                  select->yString2 + menu->yPos,
                                                  width, height, 0)) {
        return -3;
    }
    for (i = 0; i < select->visibleCount; i++) {
        index = select->scrollStart + i;
        if (index >= select->selItemCount) {
            break;
        }
        selItem = &select->selItemList[index];
        if (selItem->flags & 4) {
            if (front_maths_is_point_inside_rect_centered(
                    FRONT_MSX, FRONT_MSY,
                    item->x1 + select->x1 + selItem->xPos + frontGlobs_scrollOffsetX + menu->xPos,
                    select->yStep * i + item->y1 + select->y1 + selItem->yPos +
                        frontGlobs_scrollOffsetY + menu->yPos,
                    select->widths[0][index], select->heights[0][index],
                    menu->autoCenter)) {
                return i + select->scrollStart;
            }
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x004114f0
int front_menu_is_level_item_under_mouse(Front_Menu* menu, int index)
{
    Front_MenuItem* item;

    if (index < menu->itemCount) {
        item = menu->items[index];
        if (item->itemType == Front_MenuItem_Select) {
            return front_menu_item_select_test_string_collision(
                       menu, item, (Front_SelectData*)item->itemData) != -1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00411530
int front_menu_get_item_bounds(Front_Menu* menu, int index, int* rcX, int* rcY,
                               int* rcWidth, int* rcHeight)
{
    Front_MenuItem* item;
    Front_SliderData* slider;
    int x;
    int y;
    int width;
    int height;
    int sx;
    int sy;
    int capRight;
    int plusLo;

    if (index >= menu->itemCount) {
        return 0;
    }
    item = menu->items[index];
    switch (item->itemType) {
    case Front_MenuItem_Slider:
        x = item->centerOffHi + menu->xPos + item->x1;
        y = menu->yPos + item->y1;
        slider = (Front_SliderData*)item->itemData;
        width = formatted_text_printf_width((menu->itemFocus == index) ? item->fontHi : item->fontLo,
                                            item->banner);
        height = object_field_be4_dword_get((menu->itemFocus == index) ? menu->items[index]->fontHi
                                                                       : menu->items[index]->fontLo);
        sx = slider->x2 + item->x1 + menu->xPos;
        sy = slider->y2 + item->y1 + menu->yPos;
        capRight = (slider->imageCapRight != NULL) ? slider->imageCapRight->width : 0;
        plusLo = (slider->imagePlusLo != NULL) ? slider->imagePlusLo->width : 0;
        *rcX = (x < sx) ? x : sx;
        *rcY = (y < sy) ? y : sy;
        sx += plusLo + capRight + 104;
        *rcWidth = (x + width > sx) ? x + width : sx;
        *rcHeight = (height + y > sy + 20) ? height + y : sy + 20;
        *rcWidth -= *rcX;
        *rcHeight -= *rcY;
        return 1;
    case Front_MenuItem_Select:
        return 0;
    default:
        *rcX = menu->xPos + item->centerOffHi + item->x1;
        *rcY = menu->items[index]->y1 + menu->yPos;
        item = menu->items[index];
        if (item->isImageItem) {
            *rcWidth = ((menu->itemFocus == index) ? item->imageHi : item->imageLo)->width;
            if (menu->itemFocus == index) {
                *rcHeight = menu->items[index]->imageHi->height;
            } else {
                *rcHeight = menu->items[index]->imageLo->height;
            }
            return 1;
        }
        *rcWidth = formatted_text_printf_width((menu->itemFocus == index) ? item->fontHi : item->fontLo,
                                               item->banner);
        if (menu->itemFocus == index) {
            *rcHeight = object_field_be4_dword_get(menu->items[index]->fontHi);
        } else {
            *rcHeight = object_field_be4_dword_get(menu->items[index]->fontLo);
        }
        return 1;
    }
}

// FUNCTION: LEGORR 0x00411790
int front_menu_find_item_under_mouse(Front_Menu* menu, int* index)
{
    int i;
    int x;
    int y;
    int width;
    int height;
    int levelItem;

    i = 0;
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    levelItem = 0;
    for (;;) {
        if (!front_menu_get_item_bounds(menu, i, &x, &y, &width, &height)) {
            levelItem = front_menu_is_level_item_under_mouse(menu, i);
            if (!levelItem) {
                return 0;
            }
        }
        if (levelItem || front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY, x, y, width, height)) {
            break;
        }
        i++;
    }
    if (index != NULL) {
        *index = i;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00411840
int front_menu_was_mouse_released(void)
{
    int released;

    released = (!g_renderer_word_c && s_mouseLeftPrevious == 1);
    s_mouseLeftPrevious = g_renderer_word_c;
    return released;
}

// FUNCTION: LEGORR 0x00411870
int front_menu_item_slider_handle_input(Front_Menu* menu, Front_MenuItem* item,
                                        Front_SliderData* slider)
{
    int x;
    int y;
    float value;

    x = menu->xPos + slider->x2 + item->x1;
    y = menu->yPos + item->y1 + slider->y2;
    if (front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY, x, y, 100, 20)) {
        /* The original clamps the fraction to [0, 1] and scales it by the
         * range without adding valueMin (fild/fisub, fcom 1.0f, fcom 0.0f). */
        value = ((float)FRONT_MSX - x) * 0.01f;
        if (value > 1.0f) {
            value = 1.0f;
        }
        if (value < 0.0f) {
            value = 0.0f;
        }
        *slider->valuePtr = (int)((slider->valueMax - slider->valueMin) * value);
        return 1;
    }
    if (slider->imageMinusLo != NULL && slider->imagePlusLo != NULL) {
        if (front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY,
                                             x - slider->imageMinusLo->width - 4, y,
                                             slider->imageMinusLo->width,
                                             slider->imageMinusLo->height)) {
            if (*slider->valuePtr > slider->valueMin) {
                (*slider->valuePtr)--;
            }
            return 1;
        }
        if (front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY,
                                             slider->imageCapRight->width + 104 + x, y,
                                             slider->imagePlusLo->width,
                                             slider->imagePlusLo->height)) {
            if (*slider->valuePtr < slider->valueMax) {
                (*slider->valuePtr)++;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004119d0
int front_menu_item_check_not_in_tuto_any_tutorial_flags(Front_MenuItem* item)
{
    if (item != NULL && item->notInTuto) {
        if (nerpfunc_get_tutorial_flags(NULL)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00411a00
Front_Menu* front_menu_update_menu_items_input(float elapsed, Front_Menu* menu)
{
    Front_MenuItem* item;
    int itemIndex;
    int mouseOver;
    int handled;
    int i;
    int j;
    int selIndex;
    Front_CycleData* cycle;
    Front_TriggerData* trigger;
    Front_TextInputData* input;
    Front_SliderData* slider;
    Front_RealSliderData* realSlider;
    Front_SelectData* select;

    itemIndex = -1;
    handled = 0;
    if (menu->itemFocus >= 0) {
        item = menu->items[menu->itemFocus];
    } else {
        item = NULL;
    }
    if (frontGlobs_rockWipeFlags & FRONT_ROCKWIPE_NOINPUT) {
        return menu;
    }
    mouseOver = 1;
    if (item == NULL && menu->itemFocus >= 0) {
        menu->closed = 1;
    }
    if (front_menu_find_item_under_mouse(menu, &itemIndex)) {
        menu->itemFocus = itemIndex;
        item = menu->items[itemIndex];
    }
    if (front_menu_item_check_not_in_tuto_any_tutorial_flags(item)) {
        return menu;
    }
    if (!front_menu_was_mouse_released() || !front_menu_find_item_under_mouse(menu, &itemIndex)) {
        mouseOver = 0;
    }
    if (item == NULL && menu->itemFocus < 0) {
        return menu;
    }

    if ((s_global_current_keys[0xcd] && (s_global_previous_keys[0xcd] ^ s_global_current_keys[0xcd])) ||
        mouseOver) {
        sfx_random_play_sound_normal(0x13, 0);
        switch (item->itemType) {
        case Front_MenuItem_Cycle:
            cycle = (Front_CycleData*)item->itemData;
            handled = mouseOver;
            *cycle->valuePtr = (*cycle->valuePtr + 1) % cycle->nameCount;
            if (cycle->callback != NULL) {
                cycle->callback(*cycle->valuePtr);
            }
            break;
        case Front_MenuItem_TextInput:
            input = (Front_TextInputData*)item->itemData;
            input->caretPos++;
            if (input->caretPos > input->length - 1) {
                input->caretPos = input->length - 1;
            }
            handled = mouseOver;
            break;
        case Front_MenuItem_Slider:
            slider = (Front_SliderData*)item->itemData;
            if (!mouseOver) {
                (*slider->valuePtr)++;
                if (*slider->valuePtr > slider->valueMax) {
                    *slider->valuePtr = slider->valueMax;
                }
                handled = 0;
                if (slider->callback != NULL) {
                    slider->callback(*slider->valuePtr);
                }
            } else {
                front_menu_item_slider_handle_input(menu, item, slider);
                if (slider->callback != NULL) {
                    slider->callback(*slider->valuePtr);
                }
            }
            break;
        case Front_MenuItem_RealSlider:
            realSlider = (Front_RealSliderData*)item->itemData;
            *realSlider->valuePtr = realSlider->step + *realSlider->valuePtr;
            if (*realSlider->valuePtr > realSlider->valueMax) {
                *realSlider->valuePtr = realSlider->valueMax;
            }
            handled = mouseOver;
            if (realSlider->callback != NULL) {
                realSlider->callback(*realSlider->valuePtr);
            }
            break;
        case Front_MenuItem_Select:
            select = (Front_SelectData*)item->itemData;
            selIndex = front_menu_item_select_test_string_collision(menu, item, select);
            if (mouseOver) {
                if (selIndex == -3) {
                    if (select->scrollStart > 0) {
                        select->scrollStart--;
                    }
                } else if (selIndex == -2) {
                    if (select->scrollStart < select->selItemCount - select->visibleCount) {
                        select->scrollStart++;
                    }
                } else if (selIndex != -1) {
                    if (select->nextMenu != NULL) {
                        *select->valuePtr = selIndex;
                        if (select->callback != NULL) {
                            select->callback(elapsed, selIndex);
                        }
                        if (menu == frontGlobs_mainMenuSet->menus[3] ||
                            menu == frontGlobs_saveMenuSet->menus[0]) {
                            s_selectClickIndex = selIndex;
                            s_saveSlotToWrite = selIndex;
                        }
                        return select->nextMenu;
                    }
                    *select->valuePtr = selIndex;
                    if (select->callback != NULL) {
                        select->callback(elapsed, selIndex);
                    }
                    menu->closed = 1;
                    if (menu == frontGlobs_mainMenuSet->menus[3] ||
                        menu == frontGlobs_saveMenuSet->menus[0]) {
                        s_selectClickIndex = selIndex;
                        s_saveSlotToWrite = selIndex;
                    }
                }
            }
            break;
        }
    }

    if (s_global_current_keys[0xcb] && (s_global_previous_keys[0xcb] ^ s_global_current_keys[0xcb])) {
        switch (item->itemType) {
        case Front_MenuItem_Cycle:
            cycle = (Front_CycleData*)item->itemData;
            (*cycle->valuePtr)--;
            if (*cycle->valuePtr < 0) {
                *cycle->valuePtr = cycle->nameCount - 1;
            }
            break;
        case Front_MenuItem_TextInput:
            input = (Front_TextInputData*)item->itemData;
            input->caretPos--;
            if (input->caretPos < 0) {
                input->caretPos = 0;
            }
            break;
        case Front_MenuItem_Slider:
            slider = (Front_SliderData*)item->itemData;
            (*slider->valuePtr)--;
            if (*slider->valuePtr < slider->valueMin) {
                *slider->valuePtr = slider->valueMin;
            }
            if (slider->callback != NULL) {
                slider->callback(*slider->valuePtr);
            }
            break;
        case Front_MenuItem_RealSlider:
            realSlider = (Front_RealSliderData*)item->itemData;
            *realSlider->valuePtr -= realSlider->step;
            if (*realSlider->valuePtr < realSlider->valueMin) {
                *realSlider->valuePtr = realSlider->valueMin;
            }
            if (realSlider->callback != NULL) {
                realSlider->callback(*realSlider->valuePtr);
            }
            break;
        }
    }

    if (mouseOver && !handled) {
        switch (item->itemType) {
        case Front_MenuItem_Cycle:
            cycle = (Front_CycleData*)item->itemData;
            *cycle->valuePtr = (*cycle->valuePtr + 1) % cycle->nameCount;
            break;
        case Front_MenuItem_Trigger:
            trigger = (Front_TriggerData*)item->itemData;
            if (trigger->callback != NULL) {
                trigger->callback();
            }
            if (trigger->end) {
                menu->closed = 1;
                *trigger->valuePtr = 1;
            }
            break;
        case Front_MenuItem_Next:
            return (Front_Menu*)item->itemData;
        }
    }

    if (item->itemType == Front_MenuItem_TextInput) {
        input = (Front_TextInputData*)item->itemData;
        for (i = 0; i < 256; i++) {
            if (s_global_current_keys[i] && (s_global_previous_keys[i] ^ s_global_current_keys[i])) {
                if (i == 0x0e && input->caretPos > 0) {
                    for (j = input->caretPos; j < input->length; j++) {
                        input->text[j - 1] = input->text[j];
                    }
                    input->caretPos--;
                    input->length--;
                }
                if (input->length != input->bufferLength &&
                    input->caretPos < input->bufferLength - 1 && s_keyCharacters[i]) {
                    for (j = input->length - 1; j >= input->caretPos; j--) {
                        input->text[j + 1] = input->text[j];
                    }
                    input->text[input->caretPos] = (char)s_keyCharacters[i];
                    if (s_global_current_keys[0x2a] || s_global_current_keys[0x36]) {
                        input->text[input->caretPos] -= 0x20;
                    }
                    input->caretPos++;
                    input->length++;
                }
            }
        }
    }
    return menu;
}

// FUNCTION: LEGORR 0x00411f00
unsigned int input_key_context_check(int key)
{
    return s_keyCharacters[key];
}

// FUNCTION: LEGORR 0x00411f10
void front_menu_item_draw_slider(Front_SliderData* slider, unsigned int x, unsigned int y,
                                 unsigned int value, unsigned int range, float brightness)
{
    Area2F src;
    Point2F dest;
    float pct;

    src.x = 0.0f;
    src.y = 0.0f;
    pct = (float)((value * 100) / range);
    src.width = pct;
    src.height = 20.0f;
    dest.x = (float)x;
    dest.y = (float)y;
    image_display_scaled(slider->imageBarOn, &src, &dest, NULL);

    src.x = pct;
    src.y = 0.0f;
    src.width = 100.0f - pct;
    src.height = 20.0f;
    dest.x = (float)x + pct;
    dest.y = (float)y;
    image_display_scaled(slider->imageBarOff, &src, &dest, NULL);

    if (slider->imageCapLeft != NULL) {
        dest.x = (float)x;
        dest.y = (float)y;
        image_display_scaled(slider->imageCapLeft, NULL, &dest, NULL);
    }
    if (slider->imageCapRight != NULL) {
        dest.x = (float)x - -100.0f;
        dest.y = (float)y;
        image_display_scaled(slider->imageCapRight, NULL, &dest, NULL);
    }
    if (slider->imageMinusLo != NULL && slider->imageMinusHi != NULL) {
        dest.x = (float)x - (float)(int)(slider->imageMinusLo->width + 4);
        dest.y = (float)y;
        if (front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY, (int)dest.x, (int)dest.y,
                                             slider->imageMinusLo->width,
                                             slider->imageMinusLo->height)) {
            image_display_scaled(slider->imageMinusHi, NULL, &dest, NULL);
        } else {
            image_display_scaled(slider->imageMinusLo, NULL, &dest, NULL);
        }
    }
    if (slider->imagePlusLo != NULL && slider->imagePlusHi != NULL) {
        dest.x = ((float)(int)slider->imageCapRight->width + (float)x) - -104.0f;
        dest.y = (float)y;
        if (front_maths_is_point_inside_rect(FRONT_MSX, FRONT_MSY, (int)dest.x, (int)dest.y,
                                             slider->imagePlusLo->width,
                                             slider->imagePlusLo->height)) {
            image_display_scaled(slider->imagePlusHi, NULL, &dest, NULL);
        } else {
            image_display_scaled(slider->imagePlusLo, NULL, &dest, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x00412170
int front_menu_get_overlay_count(Front_Menu* menu)
{
    Front_MenuOverlay* overlay;
    int count;

    count = 0;
    for (overlay = menu->overlays; overlay != NULL; overlay = overlay->next) {
        count++;
    }
    return count;
}

// FUNCTION: LEGORR 0x00412190
int front_menu_should_random_play(void)
{
    int r;

    r = rand() % 400;
    return r == 0;
}

// FUNCTION: LEGORR 0x004121b0
void front_menu_update_overlays(Front_Menu* menu)
{
    Front_MenuOverlay* overlay;
    unsigned int count;
    unsigned int index;
    unsigned int width;
    unsigned int height;
    unsigned int currTime;
    int startFrame;
    int endFrame;
    Area2F area;

    if (frontGlobs_overlayImage != NULL) {
        width = anim_clone_is_lws((AnimClone*)frontGlobs_overlayImage);
        height = object_field10_get(frontGlobs_overlayImage);
        area.x = (float)frontGlobs_overlayX;
        area.y = (float)frontGlobs_overlayY;
        area.width = (float)width;
        area.height = (float)height;
        currTime = main_time_get();
        startFrame = (int)((float)(frontGlobs_overlayCurrTime - frontGlobs_overlayStartTime) * 0.025f);
        endFrame = (int)((float)(currTime - frontGlobs_overlayStartTime) * 0.025f);
        frontGlobs_overlayCurrTime = currTime;
        if (s_overlayTriggered &&
            *(int*)((char*)frontGlobs_overlayImage + 0x624) >=
                (int)*(unsigned short*)((char*)frontGlobs_overlayImage + 0x646)) {
            endFrame = startFrame;
            s_overlayIndex = -2;
        }
        if (!flic_animate(frontGlobs_overlayImage, &area, endFrame != startFrame, 0)) {
            flic_close(frontGlobs_overlayImage);
            free(frontGlobs_overlayImage);
            frontGlobs_overlayImage = NULL;
            frontGlobs_overlayStartTime = 0;
            frontGlobs_overlayCurrTime = 0;
            sound_stream_stop_and_release(0);
        }
        if (s_overlayTriggered && g_renderer_word_c) {
            s_overlayIndex = -2;
            s_overlayTriggered = 0;
        }
    } else {
        overlay = menu->overlays;
        if (overlay != NULL) {
            if (menu->playRandom == 1) {
                count = front_menu_should_random_play() ? front_menu_get_overlay_count(menu) : 0;
                if (count != 0) {
                    for (index = (unsigned int)rand() % count; index > 0; index--) {
                        overlay = overlay->next;
                    }
                    if (flic_setup(overlay->filename, (void**)&frontGlobs_overlayImage, 0)) {
                        sfx_add_to_queue(overlay->sfxType, 0);
                        frontGlobs_overlayX = overlay->x;
                        frontGlobs_overlayY = overlay->y;
                        frontGlobs_overlayStartTime = main_time_get();
                        frontGlobs_overlayCurrTime = frontGlobs_overlayStartTime;
                    }
                }
            } else if (s_overlayIndex >= 0) {
                for (index = 4 - s_overlayIndex; index != 0; index--) {
                    overlay = overlay->next;
                }
                if (flic_setup(overlay->filename, (void**)&frontGlobs_overlayImage, 0)) {
                    sfx_add_to_queue(overlay->sfxType, 0);
                    frontGlobs_overlayX = overlay->x;
                    frontGlobs_overlayY = overlay->y;
                    frontGlobs_overlayStartTime = main_time_get();
                    frontGlobs_overlayCurrTime = frontGlobs_overlayStartTime;
                }
                s_overlayTriggered = 1;
                s_overlayIndex = -1;
                s_selectClickIndex = -1;
            } else if (s_selectClickIndex >= 0) {
                s_overlayIndex = s_selectClickIndex;
                s_savedNextMenu = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00412450
void front_menu_item_draw_select_item(int x, int y, Front_Font* font,
                                      Front_SelectData* select, int selIndex,
                                      int imageIndex)
{
    struct Image* image;
    Point2F dest;


    if (select->selItemList[selIndex].flags & 1) {
        font_print_f(font, x, y, select->selItemList[selIndex].banner);

        return;
    }
    dest.x = (float)x;
    dest.y = (float)y;

    image = select->selItemList[selIndex].images[imageIndex];
    if (image != NULL && x < 640 && x > -(int)image->width && y > -(int)image->height && y < 480) {
        image_display_scaled(image, NULL, &dest, NULL);
    }
}

// FUNCTION: LEGORR 0x004124f0
void front_menu_item_draw_save_image(Front_Menu* menu, int selIndex,
                                     Front_SelectData* select, int bigSize)
{
    struct Image* image;
    char* stringParts[2];
    char buff[8];
    char* str;
    Point2F dest;
    Point2F size;

    if (menu == frontGlobs_mainMenuSet->menus[3] || menu == frontGlobs_saveMenuSet->menus[0]) {
        image = s_saveSlotImages[selIndex];
        if (image != NULL) {
            // STRING: LEGORR 0x004a237c
            sprintf(buff, "pos%d", selIndex + 1);
            str = block_get_str_copy(legoGlobs_config,
                                     string_path_join(s_engine_config_root,
                                                      // STRING: LEGORR 0x004a1f00
                                                      "Menu::SaveImage", buff, NULL));
            // STRING: LEGORR 0x004a2378
            sound_variant_split(str, stringParts, "|");
            dest.x = (float)atoi(stringParts[0]);
            dest.y = (float)atoi(stringParts[1]);
            size.x = (float)FRONT_CONFIG_INT(string_path_join(s_engine_config_root, "Menu::SaveImage",
                                                              // STRING: LEGORR 0x004a2370
                                                              "Width", NULL));
            size.y = (float)FRONT_CONFIG_INT(string_path_join(s_engine_config_root, "Menu::SaveImage",
                                                              // STRING: LEGORR 0x004a2368
                                                              "Height", NULL));
            if (bigSize) {
                dest.x -= ((float)frontGlobs_saveImageBigWidth - size.x) * 0.25f;
                dest.y -= ((float)frontGlobs_saveImageBigHeight - size.y) * 0.25f;
                size.x -= ((float)frontGlobs_saveImageBigWidth - size.x) * -0.5f;
                size.y -= ((float)frontGlobs_saveImageBigHeight - size.y) * -0.5f;
            }
            image_display_scaled(image, NULL, &dest, &size);
        }
    }
}

// FUNCTION: LEGORR 0x00412750
void front_menu_item_draw_select_text_window(Front_Menu** menu,
                                             Front_Menu** nextMenu,
                                             Front_Menu** origMenu)
{
    int i;
    int j;
    Front_MenuItem* item;
    Front_SelectData* select;

    if (*menu == frontGlobs_mainMenuSet->menus[3] || *menu == frontGlobs_saveMenuSet->menus[0]) {
        if (*menu == frontGlobs_saveMenuSet->menus[0] && s_selectClickIndex >= 0) {
            if (s_saveSlotCompletion[s_selectClickIndex] != 0) {
                s_saveMenuOverwriteShowing = 1;
            } else {
                g_front_save_menu_open = 0;
                (*menu)->closed = 1;
            }
        }
        text_buffer_reset_target(frontGlobs_saveTextWnd->textWindow);
        // STRING: LEGORR 0x004a238c
        text_buffer_append_format(frontGlobs_saveTextWnd->textWindow, "\n");
        if (s_overlayIndex == -2) {
            if (s_savedNextMenu != NULL) {
                *nextMenu = s_savedNextMenu;
            }
            s_savedNextMenu = NULL;
            s_selectHoverIndex = -1;
            s_overlayShownIndex = -1;
            goto reset;
        } else if (s_overlayIndex == -1) {
reset:
            *origMenu = NULL;
            s_selectClickIndex = -1;
            text_buffer_append_format(frontGlobs_saveTextWnd->textWindow,
                                      (*menu == frontGlobs_mainMenuSet->menus[3])
                                          ? frontGlobs_saveTextWnd->loadText
                                          : frontGlobs_saveTextWnd->saveText);
        } else {
            s_savedNextMenu = *nextMenu;
            *nextMenu = *origMenu;
            s_overlayShownIndex = s_overlayIndex;
        }
        if (s_overlayShownIndex < 0) {
            if (s_selectHoverIndex >= 0) {
                text_buffer_append_format(frontGlobs_saveTextWnd->textWindow, "\n");
                text_buffer_append_format(frontGlobs_saveTextWnd->textWindow,
                                          frontGlobs_saveTextWnd->slotText,
                                          (int)((float)s_saveSlotCompletion[s_selectHoverIndex] * 100.0f));
            }
        } else {
            text_buffer_reset_target(frontGlobs_saveTextWnd->textWindow);
            if (*menu == frontGlobs_mainMenuSet->menus[3]) {
                text_buffer_append_format(frontGlobs_saveTextWnd->textWindow,
                                          frontGlobs_saveTextWnd->slotNumberText,
                                          s_overlayShownIndex + 1, 0);
            }
        }
        if (frontGlobs_saveTextWnd->textWindow != NULL) {
            text_buffer_draw_wrapped(frontGlobs_saveTextWnd->textWindow, 0, 1.0f, NULL);
        }
    }
    if (*menu == frontGlobs_mainMenuSet->menus[3]) {
        for (i = 0; i < (*menu)->itemCount; i++) {
            item = (*menu)->items[i];
            if (item->itemType == Front_MenuItem_Select) {
                select = (Front_SelectData*)item->itemData;
                for (j = 0; j < select->selItemCount; j++) {
                    // STRING: LEGORR 0x004a2384
                    sprintf(select->selItemList[j].banner, "%s %d", frontGlobs_saveSlotFormat, j + 1);
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004129d0
void front_menu_draw_load_save_text(Front_Menu** menu)
{
    Front_LevelTextWindow* wnd;
    Point2F dest;
    Point2F size;

    wnd = frontGlobs_saveLevelWnd;
    if (*menu == frontGlobs_mainMenuSet->menus[1] || *menu == frontGlobs_mainMenuSet->menus[2]) {
        if (frontGlobs_saveLevelWnd->panelImage != NULL) {
            dest.x = frontGlobs_saveLevelWnd->panelPos[0];
            dest.y = frontGlobs_saveLevelWnd->panelPos[1];
            size.x = frontGlobs_saveLevelWnd->panelSize[0];
            size.y = frontGlobs_saveLevelWnd->panelSize[1];
            image_display_scaled(frontGlobs_saveLevelWnd->panelImage, NULL, &dest, &size);
        }
        if (wnd->textWindow != NULL) {
            text_buffer_draw_wrapped(wnd->textWindow, 0, 1.0f, NULL);
            text_buffer_reset_target(wnd->textWindow);
            text_buffer_append_format(wnd->textWindow, "\n");
            if (*menu == frontGlobs_mainMenuSet->menus[1] && wnd->levelText != NULL) {
                text_buffer_append_format(wnd->textWindow, wnd->levelText);
                text_buffer_append_format(wnd->textWindow, "\n");
            }
            if (*menu == frontGlobs_mainMenuSet->menus[2] && wnd->tutorialText != NULL) {
                text_buffer_append_format(wnd->textWindow, wnd->tutorialText);
                text_buffer_append_format(wnd->textWindow, "\n");
            }
            s_loadSaveTextShown = 1;
        }
    }
}

// FUNCTION: LEGORR 0x00412af0
void front_menu_draw_menu_image(Front_Menu* menu, int light)
{
    Area2F src;
    Point2F dest;

    if (light) {
        if (menu->menuImage != NULL) {
            if (menu->flags & 1) {
                image_display_scaled(menu->menuImage, NULL, (Point2F*)menu->imagePos, NULL);
                return;
            }
            src.x = -(float)frontGlobs_scrollOffsetX;
            src.y = -(float)frontGlobs_scrollOffsetY;
            src.width = 640.0f;
            src.height = 480.0f;
            dest.x = 0.0f;
            dest.y = 0.0f;
            image_display_scaled(menu->menuImage, &src, &dest, NULL);
        }
    } else {
        if (menu->menuImageDark != NULL) {
            if (menu->flags & 1) {
                image_display_scaled(menu->menuImageDark, NULL, (Point2F*)menu->imagePos, NULL);
                return;
            }
            src.x = -(float)frontGlobs_scrollOffsetX;
            src.y = -(float)frontGlobs_scrollOffsetY;
            src.width = 640.0f;
            src.height = 480.0f;
            dest.x = 0.0f;
            dest.y = 0.0f;
            image_display_scaled(menu->menuImageDark, &src, &dest, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x00412c00
Front_Menu* front_menu_update(float elapsed, Front_Menu* menu, int* menuTransition)
{
    Front_Menu* nextMenu;
    Front_Menu* origMenu;
    Front_MenuItem* focusItem;
    void* oldCamera;
    void* view;
    int i;
    int j;
    int index;
    int itemX;
    int itemY;
    int selCenterOff;
    int selIndex;
    int imageIndex;
    int pointerType;
    int width;
    float timer;
    float elapsedTime;
    float currTime;
    char savedChar;
    float camPos[3];
    float camDir[3];
    float camUp[3];
    Point2F dest;
    char buff[128];
    Front_CycleData* cycle;
    Front_SelectItem* selItem;
    Front_TextInputData* input;
    Front_SliderData* slider;
    Front_RealSliderData* realSlider;
    Front_SelectData* select;
    Front_MenuItem* item;
    int x;
    int centerOff;
    int y;

    sav_build_ctx_target();
    pointerType = pointer_get_current_type();
    view = legoGlobs_viewMain;
    oldCamera = (void*)viewport_get_camera((struct Viewport*)view);
    s_selectClickIndex = -1;
    menu->itemFocus = -1;
    if (menuTransition != NULL) {
        *menuTransition = 0;
    }
    pointer_set_current_if_timer_finished(0);
    nextMenu = menu;
    origMenu = menu;
    if (!s_saveMenuOverwriteShowing && s_savedNextMenu == NULL) {
        nextMenu = front_menu_update_menu_items_input(elapsed, menu);
    }
    if (menu == frontGlobs_saveMenuSet->menus[0] && s_selectClickIndex >= 0 && s_overwriteAnswer < 0) {
        menu->closed = 0;
        nextMenu->closed = 0;
    }
    if (s_overlayIndex >= 0) {
        nextMenu = origMenu;
    }
    x = menu->xPos;
    y = menu->yPos;

    if (frontGlobs_rockWipeAnim != NULL && (frontGlobs_rockWipeFlags & FRONT_ROCKWIPE_ANIMATING)) {
        viewport_clear((struct Viewport*)view, 1);
        ddiscene_scene_object_attach_or_unlink(frontGlobs_rockWipeAnim, legoGlobs_rootCont);
        oldCamera = (void*)viewport_get_camera((struct Viewport*)view);
        ddiscene_object_position_get(oldCamera, NULL, camPos);
        ddiscene_object_facing_pair_get(oldCamera, NULL, camDir, camUp);
        ddiscene_object_position_set(oldCamera, NULL, 0.0f, 0.0f, 5.0f);
        ddiscene_object_pose_method_dispatch(oldCamera, NULL, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
        ddiscene_frame_update_and_callbacks(frontGlobs_rockWipeAnim, frontGlobs_rockWipeTimer);
    }

    front_menu_draw_menu_image(menu, 1);
    if (menu != frontGlobs_saveMenuSet->menus[0]) {
        front_menu_update_overlays(menu);
    }
    front_menu_item_draw_select_text_window(&menu, &nextMenu, &origMenu);

    if (menu->displayTitle) {
        width = formatted_text_printf_width(menu->menuFont, front_util_replace_text_spaces(menu->fullName));
        font_print_f(menu->menuFont, s_display_w / 2 - ((unsigned int)width >> 1), menu->anchoredY + y,
                     front_util_replace_text_spaces(menu->fullName));
    }

    s_selectHoverIndex = -1;
    for (i = 0; i < menu->itemCount; i++) {
        if (menu->itemFocus == i && !s_saveMenuOverwriteShowing &&
            !front_menu_item_check_not_in_tuto_any_tutorial_flags(menu->items[i])) {
            item = menu->items[i];
            itemY = item->y1;
            itemX = item->x1;
            centerOff = item->centerOffHi;
            if (item->fontHi != NULL) {
                font_print_f(item->fontHi, centerOff + itemX + x, itemY + y, "%s",
                             front_util_replace_text_spaces(item->banner));
            }
            focusItem = menu->items[menu->itemFocus];
            switch (focusItem->itemType) {
            case Front_MenuItem_Cycle:
                cycle = (Front_CycleData*)focusItem->itemData;
                if (item->fontHi != NULL) {
                    font_print_f(item->fontHi, cycle->x2 + centerOff + itemX + x, cycle->y2 + itemY + y,
                                 front_util_replace_text_spaces(cycle->nameList[*cycle->valuePtr]));
                }
                break;
            case Front_MenuItem_TextInput:
                input = (Front_TextInputData*)focusItem->itemData;
                savedChar = input->text[input->caretPos];
                s_textInputBlink++;
                if (s_textInputBlink != 0 && (s_textInputBlink / 2) % 2) {
                    input->text[input->caretPos] = '_';
                }
                if (item->fontHi != NULL) {
                    font_print_f(item->fontHi, input->x2 + centerOff + itemX + x, input->y2 + itemY + y, "%s",
                                 front_util_replace_text_spaces(input->text));
                }
                if (s_textInputBlink != 0 && (s_textInputBlink / 2) % 2) {
                    input->text[input->caretPos] = savedChar;
                }
                break;
            case Front_MenuItem_Slider:
                slider = (Front_SliderData*)focusItem->itemData;
                front_menu_item_draw_slider(slider, slider->x2 + itemX + x, slider->y2 + itemY + y,
                                            *slider->valuePtr - slider->valueMin,
                                            slider->valueMax - slider->valueMin, 1.0f);
                break;
            case Front_MenuItem_RealSlider:
                realSlider = (Front_RealSliderData*)focusItem->itemData;
                if (item->fontHi != NULL) {
                    font_print_f(item->fontHi, realSlider->x2 + centerOff + itemX + x,
                                 realSlider->y2 + itemY + y, "%.2f", (double)*realSlider->valuePtr);
                }
                break;
            case Front_MenuItem_Select:
                select = (Front_SelectData*)menu->items[i]->itemData;
                selIndex = front_menu_item_select_test_string_collision(menu, menu->items[i], select);
                s_selectHoverIndex = selIndex;
                for (j = 0; j < select->visibleCount; j++) {
                    index = select->scrollStart + j;
                    if (index >= select->selItemCount) {
                        break;
                    }
                    selCenterOff = -(select->widths[0][index] / 2);
                    selItem = &select->selItemList[index];
                    imageIndex = (selItem->flags & 4) ? 0 : 2;
                    if (!menu->autoCenter) {
                        selCenterOff = 0;
                    }
                    if (selIndex == index) {
                        front_menu_item_draw_select_item(
                            select->x1 + selItem->xPos + selCenterOff + itemX + x + frontGlobs_scrollOffsetX,
                            select->yStep * j + select->y1 + selItem->yPos + itemY + y + frontGlobs_scrollOffsetY,
                            item->fontHi, select, index, imageIndex);
                        if (select->callback != NULL) {
                            select->callback(elapsed, selIndex);
                        }
                        front_menu_item_draw_save_image(menu, j + select->scrollStart, select, 1);
                    } else {
                        if (!imageIndex) {
                            imageIndex = 1;
                        }
                        front_menu_item_draw_select_item(
                            select->x1 + selItem->xPos + selCenterOff + itemX + x + frontGlobs_scrollOffsetX,
                            select->yStep * j + select->y1 + selItem->yPos + itemY + y + frontGlobs_scrollOffsetY,
                            item->fontLo, select, index, imageIndex);
                        front_menu_item_draw_save_image(menu, j + select->scrollStart, select, 0);
                    }
                }
                if (selIndex == -2 && select->scrollStart < select->selItemCount - select->visibleCount) {
                    if (item->fontHi != NULL) {
                        font_print_f(item->fontHi, select->xString1 + x, select->yString1 + y, select->string1);
                    }
                } else if (select->scrollStart < select->selItemCount - select->visibleCount) {
                    if (item->fontLo != NULL) {
                        font_print_f(item->fontLo, select->xString1 + x, select->yString1 + y, select->string1);
                    }
                }
                if (selIndex == -3) {
                    if (select->scrollStart > 0 && item->fontHi != NULL) {
                        font_print_f(item->fontHi, select->xString2 + x, select->yString2 + y, select->string2);
                    }
                } else if (select->scrollStart > 0 && item->fontLo != NULL) {
                    font_print_f(item->fontLo, select->xString2 + x, select->yString2 + y, select->string2);
                }
                break;
            }
        } else {
            item = menu->items[i];
            itemX = item->x1;
            itemY = item->y1;
            centerOff = item->centerOffLo;
            if (item->fontLo != NULL) {
                font_print_f(menu->items[i]->fontLo, centerOff + itemX + x, itemY + y, "%s",
                             front_util_replace_text_spaces(item->banner));
            }
            item = menu->items[i];
            switch (item->itemType) {
            case Front_MenuItem_Cycle:
                cycle = (Front_CycleData*)item->itemData;
                if (item->fontLo != NULL) {
                    font_print_f(item->fontLo, cycle->x2 + centerOff + itemX + x, cycle->y2 + itemY + y,
                                 front_util_replace_text_spaces(cycle->nameList[*cycle->valuePtr]));
                }
                break;
            case Front_MenuItem_TextInput:
                input = (Front_TextInputData*)item->itemData;
                if (item->fontLo != NULL) {
                    font_print_f(item->fontLo, input->x2 + centerOff + itemX + x, input->y2 + itemY + y, "%s",
                                 front_util_replace_text_spaces(input->text));
                }
                break;
            case Front_MenuItem_Slider:
                slider = (Front_SliderData*)item->itemData;
                front_menu_item_draw_slider(slider, slider->x2 + itemX + x, slider->y2 + itemY + y,
                                            *slider->valuePtr - slider->valueMin,
                                            slider->valueMax - slider->valueMin, 0.25f);
                break;
            case Front_MenuItem_RealSlider:
                realSlider = (Front_RealSliderData*)item->itemData;
                if (item->fontLo != NULL) {
                    font_print_f(item->fontLo, realSlider->x2 + centerOff + itemX + x,
                                 realSlider->y2 + itemY + y, "%.2f", (double)*realSlider->valuePtr);
                }
                break;
            case Front_MenuItem_Select:
                select = (Front_SelectData*)item->itemData;
                for (j = 0; j < select->visibleCount; j++) {
                    index = select->scrollStart + j;
                    if (index >= select->selItemCount) {
                        break;
                    }
                    selCenterOff = -(select->widths[0][index] / 2);
                    selItem = &select->selItemList[index];
                    imageIndex = (selItem->flags & 4) ? 0 : 2;
                    if (!menu->autoCenter) {
                        selCenterOff = 0;
                    }
                    if (!imageIndex) {
                        imageIndex = 1;
                    }
                    front_menu_item_draw_select_item(
                        select->x1 + selItem->xPos + selCenterOff + itemX + x + frontGlobs_scrollOffsetX,
                        select->yStep * j + select->y1 + selItem->yPos + itemY + y + frontGlobs_scrollOffsetY,
                        item->fontLo, select, index, imageIndex);
                    front_menu_item_draw_save_image(menu, j + select->scrollStart, select, 0);
                }
                if (select->scrollStart < select->selItemCount - select->visibleCount && item->fontLo != NULL) {
                    font_print_f(item->fontLo, select->xString1 + x, select->yString1 + y, select->string1);
                }
                if (select->scrollStart > 0 && item->fontLo != NULL) {
                    font_print_f(item->fontLo, select->xString2 + x, select->yString2 + y, select->string2);
                }
                break;
            }
        }
    }

    front_menu_draw_load_save_text(&menu);

    for (i = 0; i < menu->itemCount; i++) {
        item = menu->items[i];
        if (item->isImageItem) {
            if (menu->itemFocus == i) {
                itemX = item->x1 + menu->xPos;
                itemY = item->y1 + menu->yPos;
                if (item->imageHi != NULL) {
                    dest.x = (float)itemX;
                    dest.y = (float)itemY;
                    image_display_scaled(item->imageHi, NULL, &dest, NULL);
                }
                tooltip_activate(item->toolTipType);
            } else {
                itemX = item->x1 + menu->xPos;
                itemY = item->y1 + menu->yPos;
                if (item->imageLo != NULL) {
                    dest.x = (float)itemX;
                    dest.y = (float)itemY;
                    image_display_scaled(item->imageLo, NULL, &dest, NULL);
                }
            }
        }
    }

    if ((s_engine_feature_word & 0x200000) && frontGlobs_versionFont != NULL &&
        frontGlobs_versionString != NULL && s_application_mode == 0) {
        font_print_f(frontGlobs_versionFont, 545, 450, frontGlobs_versionString);
    }

    front_util_print_f(NULL, 0, 0, NULL);

    if (menu == frontGlobs_saveMenuSet->menus[0] && s_saveMenuOverwriteShowing) {
        sprintf(buff, frontGlobs_overwriteText, s_saveSlotToWrite + 1);
        s_overwriteAnswer = lego_menu_option_panel_texture_update(frontGlobs_overwriteTitle, buff,
                                                                  frontGlobs_overwriteOK,
                                                                  frontGlobs_overwriteCancel);
        if (s_overwriteAnswer == 0) {
            g_front_save_menu_open = 1;
            menu->closed = 1;
            s_saveMenuOverwriteShowing = 0;
            s_saveSlotToWrite = -1;
        } else if (s_overwriteAnswer == 1) {
            g_front_save_menu_open = 0;
            menu->closed = 1;
            s_saveMenuOverwriteShowing = 0;
        }
    }

    if (frontGlobs_rockWipeAnim != NULL && (frontGlobs_rockWipeFlags & FRONT_ROCKWIPE_ANIMATING)) {
        scene_render_lock_toggle(frontGlobs_rockWipeLight, 0);
        viewport_render((struct Viewport*)view, (struct Container*)legoGlobs_rootCont, 0.0f);
        scene_render_lock_toggle(frontGlobs_rockWipeLight, 1);
        ddiscene_object_position_set(oldCamera, NULL, camPos[0], camPos[1], camPos[2]);
        ddiscene_object_pose_method_dispatch(oldCamera, NULL, camDir[0], camDir[1], camDir[2],
                                             camUp[0], camUp[1], camUp[2]);
        ddiscene_scene_object_attach_or_unlink(frontGlobs_rockWipeAnim, NULL);
        timer = (float)ddiscene_object_frame_position_get(frontGlobs_rockWipeAnim);
        currTime = (float)main_time_get();
        elapsedTime = (currTime - frontGlobs_rockWipeLastUpdateTime) * 0.001f * 25.0f;
        elapsedTime = elapsedTime + elapsedTime;
        if (elapsedTime > 3.0f) {
            elapsedTime = 3.0f;
        }
        if (frontGlobs_rockWipeTimer <= timer * 0.5f &&
            elapsedTime + frontGlobs_rockWipeTimer >= timer * 0.5f) {
            if (menuTransition != NULL) {
                *menuTransition = 1;
            }
            s_overlayIndex = -1;
            s_selectClickIndex = -1;
            s_savedNextMenu = NULL;
            s_overlayTriggered = 0;
            if (frontGlobs_overlayImage != NULL) {
                frontGlobs_overlayStartTime = 0;
                frontGlobs_overlayCurrTime = 0;
            }
        }
        if (frontGlobs_rockWipeTimer > timer) {
            front_rock_wipe_stop();
        }
        frontGlobs_rockWipeTimer = elapsedTime + frontGlobs_rockWipeTimer;
        frontGlobs_rockWipeLastUpdateTime = currTime;
    }

    pointer_draw_pointer(s_renderer_word_a, s_renderer_word_b);
    pointer_set_current_if_timer_finished(pointerType);
    return nextMenu;
}

// FUNCTION: LEGORR 0x00413970
void front_menu_update_mouse_position(Front_Menu* menu, float elapsed)
{
    float msy;
    float speed;
    int height;

    if (menu->menuImage != NULL && (menu->flags & 2)) {
        msy = (float)FRONT_MSY;
        if (msy > 280.0f || msy < 200.0f) {
            height = menu->menuImage->height;
            if (msy < 480.0f && !g_renderer_word_c && !s_window_mouse_word_40) {
                speed = msy * 0.208333328f - 50.0f;
                speed = speed * speed * 0.02f;
                if (msy < 240.0f) {
                    speed = -speed;
                }
                frontGlobs_scrollOffsetY += (int)(speed * -0.25f);
            }
            if (frontGlobs_scrollOffsetY > 0) {
                frontGlobs_scrollOffsetY = 0;
            }
            if (frontGlobs_scrollOffsetY - 480 < -height) {
                frontGlobs_scrollOffsetY = 480 - height;
            }
        }
    } else {
        frontGlobs_scrollOffsetX = 0;
        frontGlobs_scrollOffsetY = 0;
    }
}

// FUNCTION: LEGORR 0x00413a60
void front_menu_load_slot_images(void)
{
    int i;
    struct Image** image;
    int* completion;
    unsigned int k;

    /* buff and saveData are block-scoped: the original's 0xb8-byte frame
     * holds either one. */
    i = 0;
    for (image = s_saveSlotImages; image < s_saveSlotImages + 5; image++) {
        char buff[128];

        // STRING: LEGORR 0x004a2398
        sprintf(buff, "%s\\%d.dat",
                block_get_str_copy(legoGlobs_config,
                                   string_path_join(s_engine_config_root, "Menu::SaveImage",
                                                    // STRING: LEGORR 0x004a23a4
                                                    "Path", NULL)),
                i);
        *image = image_load_bmp_scaled(buff, 0, 0);
        i++;
    }
    i = 0;
    for (completion = s_saveSlotCompletion; completion < s_saveSlotCompletion + 5; completion++) {
        /* A Front_SaveData (0xb8 bytes, defined with part 2): [6] is
         * missionsCount, [13] missionsTable. Each mission record is 0x3190
         * bytes, its reward score at +0x2fb8. */
        int saveData[0xb8 / 4] = { 0 };

        save_read_slot_target(i, saveData, 1);
        *completion = 0;
        for (k = 8; k < (unsigned int)saveData[6]; k++) {
            *completion += (*(float*)((char*)saveData[13] + k * 0x3190 + 0x2fb8) != 0.0f) ? 4 : 0;
        }
        free((void*)saveData[13]);
        i++;
    }
}

// FUNCTION: LEGORR 0x00413b50
void front_menu_free_slot_images(void)
{
    struct Image** image;

    for (image = s_saveSlotImages; image < s_saveSlotImages + 5; image++) {
        if (*image != NULL) {
            image_surface_release(*image);
        }
    }
}

// FUNCTION: LEGORR 0x00413b80
void front_run_screen_menu(Front_Menu* menu)
{
    int transition;
    int menuTransition;
    Front_Menu* nextMenu;
    float elapsed;
    unsigned int lastTime;
    unsigned int currTime;
    void* level;
    char buff[128];

    transition = 0;
    menuTransition = 0;
    elapsed = 1.0f;
    front_menu_load_slot_images();
    s_savedNextMenu = NULL;
    s_overlayIndex = -1;
    s_selectClickIndex = -1;
    s_overlayShownIndex = -1;
    s_overwriteAnswer = -1;
    s_saveSlotToWrite = -1;
    s_overlayTriggered = 0;
    s_saveMenuOverwriteShowing = 0;
    s_loadSaveTextShown = 0;
    g_front_save_menu_result = 1;
    g_front_save_menu_open = 1;
    frontGlobs_overlayImage = NULL;
    lastTime = timeGetTime();
    while (!menu->closed) {
        if (transition) {
            front_menu_update(elapsed, menu, &menuTransition);
        } else {
            nextMenu = front_menu_update(elapsed, menu, NULL);
        }
        if (menu == frontGlobs_mainMenuSet->menus[0] && nextMenu != menu) {
            frontGlobs_f_557028 = -1;
            save_menu_refresh_target(FRONT_FLOAT_ARG(elapsed), -1);
        }
        front_menu_update_mouse_position(menu, elapsed);
        tooltip_update(s_renderer_word_a, s_renderer_word_b, elapsed);
        sfx_update(elapsed);
        main_loop_frame_pump(0);
        currTime = timeGetTime();
        elapsed = (float)(currTime - lastTime) * 0.025f;
        lastTime = currTime;
        if (!transition && nextMenu != menu) {
            front_rock_wipe_play();
            transition = 1;
            if (frontGlobs_rockWipeAnim == NULL) {
                menuTransition = 1;
            }
        }
        if (transition && menuTransition) {
            if (menu->menuImage != nextMenu->menuImage && frontGlobs_overlayImage != NULL) {
                flic_close(frontGlobs_overlayImage);
                free(frontGlobs_overlayImage);
                sound_stream_stop_and_release(0);
                frontGlobs_overlayImage = NULL;
                frontGlobs_overlayStartTime = 0;
                frontGlobs_overlayCurrTime = 0;
            }
            frontGlobs_scrollOffsetY = 0;
            frontGlobs_scrollOffsetX = 0;
            transition = 0;
            menuTransition = 0;
            menu = nextMenu;
        }
    }
    front_menu_free_slot_images();
    if (nextMenu == frontGlobs_saveMenuSet->menus[0] && s_saveSlotToWrite >= 0) {
        level = reward_current_level_get();
        if (level != NULL && *(void**)((char*)reward_current_level_get() + 4) != NULL) {
            level = reward_current_level_get();
            sprintf(buff, "%s\\%d.dat",
                    block_get_str_copy(legoGlobs_config,
                                       string_path_join(s_engine_config_root, "Menu::SaveImage",
                                                        "Path", NULL)),
                    s_saveSlotToWrite);
            front_image_save_bmp_adapter((char*)level + 8, buff);
        }
    }
    if (frontGlobs_overlayImage != NULL) {
        flic_close(frontGlobs_overlayImage);
        free(frontGlobs_overlayImage);
        sound_stream_stop_and_release(0);
        frontGlobs_overlayImage = NULL;
        frontGlobs_overlayStartTime = 0;
        frontGlobs_overlayCurrTime = 0;
    }
    front_rock_wipe_stop();
}

// FUNCTION: LEGORR 0x00413e20
void front_screen_menu_loop(Front_MenuSet* menuSet, int menuIndex)
{
    int i;

    for (i = 0; i < menuSet->menuCount; i++) {
        menuSet->menus[i]->closed = 0;
    }
    front_run_screen_menu(menuSet->menus[menuIndex]);
}

// FUNCTION: LEGORR 0x00413e60
int front_menu_item_parse_type_string(const char* itemTypeName)
{
    // STRING: LEGORR 0x004a23dc
    if (_strcmpi(itemTypeName, "cycle") == 0) {
        return Front_MenuItem_Cycle;
    }
    // STRING: LEGORR 0x004a23d4
    if (_strcmpi(itemTypeName, "trigger") == 0) {
        return Front_MenuItem_Trigger;
    }
    // STRING: LEGORR 0x004a23c8
    if (_strcmpi(itemTypeName, "textinput") == 0) {
        return Front_MenuItem_TextInput;
    }
    // STRING: LEGORR 0x004a23c0
    if (_strcmpi(itemTypeName, "slider") == 0) {
        return Front_MenuItem_Slider;
    }
    // STRING: LEGORR 0x004a23b4
    if (_strcmpi(itemTypeName, "realslider") == 0) {
        return Front_MenuItem_RealSlider;
    }
    // STRING: LEGORR 0x004a23ac
    return (_strcmpi(itemTypeName, "next") == 0) ? Front_MenuItem_Next : -1;
}

// FUNCTION: LEGORR 0x00413f00
char* mission_replace_char(char* text, char from, char to)
{
    char* original;

    original = text;
    while (*text != '\0') {
        if (*text == from) {
            *text = to;
        }
        text++;
    }
    return original;
}

// FUNCTION: LEGORR 0x00413f30
int front_menu_overlay_get_type(Front_MenuOverlay* overlay)
{
    char* s;

    s = overlay->filename;
    while (*s != '.') {
        s++;
    }
    // STRING: LEGORR 0x004a23ec
    if (_strcmpi(".avi", s) == 0) {
        return 1;
    }
    if (_strcmpi(".bmp", s) == 0) {
        return 2;
    }
    // STRING: LEGORR 0x004a23e4
    return (_strcmpi(".lws", s) == 0) ? 3 : 0;
}

// FUNCTION: LEGORR 0x00413f90
Front_MenuOverlay* front_menu_create_overlay(const char* filename,
                                             Front_MenuOverlay** list,
                                             int x, int y, int sfxType)
{
    Front_MenuOverlay* overlay;

    overlay = (Front_MenuOverlay*)malloc(sizeof(Front_MenuOverlay));
    memset(overlay, 0, sizeof(Front_MenuOverlay));
    if (strnicmp(filename, s_overlayNullName, 4) != 0) {
        if (filename != NULL) {
            overlay->filename = front_util_string_copy(filename);
        }
        overlay->overlayType = front_menu_overlay_get_type(overlay);
        overlay->field_1c = 0;
        overlay->next = *list;
        overlay->x = x;
        overlay->y = y;
        overlay->sfxType = sfxType;
        *list = overlay;
    }
    return overlay;
}

// FUNCTION: LEGORR 0x00414010
void front_menu_item_load_slider_images(int arrayLength, char** stringParts,
                                        struct Image** images)
{
    int i;

    if (arrayLength == 16) {
        for (i = 0; i < 8; i++) {
            images[i] = front_cache_load_image(stringParts[i + 8]);
            if (images[i] != NULL) {
                resource_record_add_ref(images[i]);
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            images[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00414070
Front_MenuSet* front_menu_set_create(int menuCount)
{
    Front_MenuSet* menuSet;

    menuSet = (Front_MenuSet*)malloc(sizeof(Front_MenuSet));
    memset(menuSet, 0, sizeof(Front_MenuSet));
    menuSet->menus = (Front_Menu**)malloc(menuCount * sizeof(Front_Menu*));
    memset(menuSet->menus, 0, menuCount * sizeof(Front_Menu*));
    menuSet->menuCount = menuCount;
    return menuSet;
}

// FUNCTION: LEGORR 0x004140c0
Front_MenuSet* front_load_menu_set(struct BlockRec* config, const char* menuName, ...)
{
    char menuPath[1024];
    char itemPath[1024];
    char buff[1024];
    char* stringParts[100];
    struct Image* sliderImages[8];
    char* menuTitle;
    char* menuFullName;
    char* positionStr;
    char* itemString;
    char* anchored;
    Front_MenuSet* menuSet;
    Front_Font* menuFont;
    Front_Font* loFont;
    Front_Font* hiFont;
    void* itemData;
    va_list args;
    int menuCount;
    int itemCount;
    int numParts;
    int autoCenter;
    int displayTitle;
    int centerOffset;
    int xPos;
    int yPos;
    int sfxType;
    int i;
    int j;
    int k;

    va_start(args, menuName);
    // STRING: LEGORR 0x004a24d8
    sprintf(menuPath, "Menu::%s", menuName);
    menuCount = FRONT_CONFIG_INT(string_path_join(s_engine_config_root, menuPath,
                                                  // STRING: LEGORR 0x004a24cc
                                                  "MenuCount", NULL));
    menuSet = front_menu_set_create(menuCount);
    for (i = 0; i < menuCount; i++) {
        centerOffset = 0;
        // STRING: LEGORR 0x004a24c0
        sprintf(itemPath, "%s::Menu%i", menuPath, i + 1);
        menuTitle = block_get_str_copy(legoGlobs_config,
                                       // STRING: LEGORR 0x004a24b8
                                       string_path_join(s_engine_config_root, itemPath, "Title", NULL));
        menuFullName = block_get_str_copy(legoGlobs_config,
                                          // STRING: LEGORR 0x004a24ac
                                          string_path_join(s_engine_config_root, itemPath, "FullName", NULL));
        positionStr = block_get_str_copy(legoGlobs_config,
                                         // STRING: LEGORR 0x004a24a0
                                         string_path_join(s_engine_config_root, itemPath, "Position", NULL));
        sound_variant_split(positionStr, stringParts, s_config_rgb_separator);
        xPos = atoi(stringParts[0]);
        yPos = atoi(stringParts[1]);
        if (positionStr != NULL) {
            free(positionStr);
        }
        menuFont = front_cache_load_font(block_get_strptr(legoGlobs_config,
                                                          // STRING: LEGORR 0x004a2494
                                                          string_path_join(s_engine_config_root, itemPath, "MenuFont", NULL)));
        autoCenter = block_get_bool_path(legoGlobs_config,
                                         // STRING: LEGORR 0x004a2488
                                         string_path_join(s_engine_config_root, itemPath, "AutoCenter", NULL)) == 1;
        displayTitle = block_get_bool_path(legoGlobs_config,
                                           // STRING: LEGORR 0x004a2478
                                           string_path_join(s_engine_config_root, itemPath, "DisplayTitle", NULL)) != 0;
        anchored = block_get_str_copy(legoGlobs_config,
                                      // STRING: LEGORR 0x004a246c
                                      string_path_join(s_engine_config_root, itemPath, "Anchored", NULL));
        if (autoCenter && menuTitle != NULL && *menuTitle != '\0') {
            centerOffset = -((int)formatted_text_printf_width(menuFont, menuTitle) / 2);
        }
        menuSet->menus[i] = front_menu_create_menu(
            mission_replace_char(menuTitle, '_', ' '), mission_replace_char(menuFullName, '_', ' '),
            menuFont, xPos, yPos, autoCenter, displayTitle, centerOffset,
            block_get_bool_path(legoGlobs_config,
                                // STRING: LEGORR 0x004a2460
                                string_path_join(s_engine_config_root, itemPath, "CanScroll", NULL)) == 1,
            anchored);
        front_menu_load_menu_image(menuSet->menus[i],
                                   block_get_strptr(legoGlobs_config,
                                                    // STRING: LEGORR 0x004a2454
                                                    string_path_join(s_engine_config_root, itemPath, "MenuImage", NULL)),
                                   1);
        front_menu_load_menu_image(menuSet->menus[i],
                                   block_get_strptr(legoGlobs_config,
                                                    // STRING: LEGORR 0x004a2444
                                                    string_path_join(s_engine_config_root, itemPath, "MenuImageDark", NULL)),
                                   0);
        sprintf(menuSet->menus[i]->name, "%s", itemPath);
        menuSet->menus[i]->playRandom =
            block_get_bool_path(legoGlobs_config,
                                // STRING: LEGORR 0x004a2438
                                string_path_join(s_engine_config_root, menuSet->menus[i]->name, "PlayRandom", NULL));
        free(menuTitle);
    }

    for (i = 0; i < menuCount; i++) {
        autoCenter = menuSet->menus[i]->autoCenter;
        sprintf(itemPath, "%s::Menu%i", menuPath, i + 1);
        loFont = front_cache_load_font(block_get_strptr(legoGlobs_config,
                                                        // STRING: LEGORR 0x004a2430
                                                        string_path_join(s_engine_config_root, itemPath, "LoFont", NULL)));
        hiFont = front_cache_load_font(block_get_strptr(legoGlobs_config,
                                                        // STRING: LEGORR 0x004a2428
                                                        string_path_join(s_engine_config_root, itemPath, "HiFont", NULL)));
        itemCount = FRONT_CONFIG_INT(string_path_join(s_engine_config_root, itemPath,
                                                      // STRING: LEGORR 0x004a241c
                                                      "ItemCount", NULL));

        for (j = 1;; j++) {
            // STRING: LEGORR 0x004a2410
            sprintf(buff, "Overlay%i", j);
            itemString = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, itemPath, buff, NULL));
            if (itemString == NULL) {
                if (s_engine_feature_word & 0x1000000) {
                    break;
                }
                // STRING: LEGORR 0x004a2404
                sprintf(buff, "!Overlay%i", j);
                itemString = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, itemPath, buff, NULL));
                if (itemString == NULL) {
                    break;
                }
            }
            sound_variant_split(itemString, stringParts, s_config_rgb_separator);
            if (!sfx_get_type(stringParts[1], &sfxType)) {
                sfxType = 0;
            }
            front_menu_create_overlay(stringParts[0], &menuSet->menus[i]->overlays, atoi(stringParts[2]),
                                      atoi(stringParts[3]), sfxType);
            if (itemString != NULL) {
                free(itemString);
            }
        }

        for (j = 0; j < itemCount; j++) {
            // STRING: LEGORR 0x004a23fc
            sprintf(buff, "Item%i", j + 1);
            if (itemString != NULL) {
                free(itemString);
            }
            itemString = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, itemPath, buff, NULL));
            numParts = sound_variant_split(itemString, stringParts, s_config_rgb_separator);
            switch (front_menu_item_parse_type_string(stringParts[0])) {
            case Front_MenuItem_Cycle:
            {
                int* valuePtr = va_arg(args, int*);
                Front_IntCallback callback = va_arg(args, Front_IntCallback);

                itemData = front_menu_item_create_cycle(atoi(stringParts[6]), valuePtr, atoi(stringParts[3]),
                                                        atoi(stringParts[4]), callback);
                front_menu_add_menu_item(menuSet->menus[i],
                                         front_menu_item_create_banner_item(
                                             mission_replace_char(stringParts[5], '_', ' '), loFont, hiFont,
                                             atoi(stringParts[1]), atoi(stringParts[2]), Front_MenuItem_Cycle,
                                             autoCenter, itemData, 0));
                for (k = 0; k < atoi(stringParts[6]); k++) {
                    front_menu_item_add_cycle_item((Front_CycleData*)itemData,
                                                   mission_replace_char(stringParts[7 + k], '_', ' '));
                }
                break;
            }
            case Front_MenuItem_Trigger:
                if (numParts == 8) {
                    int* valuePtr = va_arg(args, int*);
                    Front_TriggerCallback callback = va_arg(args, Front_TriggerCallback);

                    itemData = front_menu_item_create_trigger(valuePtr, atoi(stringParts[7]) == 1, callback);
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_image_item(
                                                 s_config_angle_empty, loFont, hiFont, stringParts[3],
                                                 stringParts[4], atoi(stringParts[1]), atoi(stringParts[2]),
                                                 Front_MenuItem_Trigger, autoCenter, stringParts[6], itemData));
                } else if (numParts == 5) {
                    int* valuePtr = va_arg(args, int*);
                    Front_TriggerCallback callback = va_arg(args, Front_TriggerCallback);

                    itemData = front_menu_item_create_trigger(valuePtr, atoi(stringParts[4]) == 1, callback);
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_banner_item(
                                                 mission_replace_char(stringParts[3], '_', ' '), loFont, hiFont,
                                                 atoi(stringParts[1]), atoi(stringParts[2]),
                                                 Front_MenuItem_Trigger, autoCenter, itemData, 0));
                }
                break;
            case Front_MenuItem_Slider:
            {
                int* valuePtr = va_arg(args, int*);
                Front_IntCallback callback = va_arg(args, Front_IntCallback);

                front_menu_item_load_slider_images(numParts, stringParts, sliderImages);
                itemData = front_menu_item_create_slider(
                    valuePtr, atoi(stringParts[6]), atoi(stringParts[7]), atoi(stringParts[3]),
                    atoi(stringParts[4]), callback, sliderImages[0], sliderImages[1], sliderImages[2],
                    sliderImages[3], sliderImages[4], sliderImages[5], sliderImages[6], sliderImages[7]);
                front_menu_add_menu_item(menuSet->menus[i],
                                         front_menu_item_create_banner_item(
                                             mission_replace_char(stringParts[5], '_', ' '), loFont, hiFont,
                                             atoi(stringParts[1]), atoi(stringParts[2]), Front_MenuItem_Slider,
                                             autoCenter, itemData, 0));
                break;
            }
            case Front_MenuItem_RealSlider:
            {
                float* valuePtr = va_arg(args, float*);
                Front_RealCallback callback = va_arg(args, Front_RealCallback);

                itemData = front_menu_item_create_real_slider(
                    valuePtr, (float)atof(stringParts[6]), (float)atof(stringParts[7]),
                    (float)atof(stringParts[8]), atoi(stringParts[3]), atoi(stringParts[4]), callback);
                front_menu_add_menu_item(menuSet->menus[i],
                                         front_menu_item_create_banner_item(
                                             mission_replace_char(stringParts[5], '_', ' '), loFont, hiFont,
                                             atoi(stringParts[1]), atoi(stringParts[2]),
                                             Front_MenuItem_RealSlider, autoCenter, itemData, 0));
                break;
            }
            case Front_MenuItem_Next:
                if (numParts == 8) {
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_image_item(
                                                 s_config_angle_empty, loFont, hiFont, stringParts[3],
                                                 stringParts[4], atoi(stringParts[1]), atoi(stringParts[2]),
                                                 Front_MenuItem_Next, autoCenter, stringParts[5],
                                                 menuSet->menus[atoi(stringParts[7] + 4) - 1]));
                } else if (numParts == 6) {
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_banner_item(
                                                 mission_replace_char(stringParts[3], '_', ' '), loFont, hiFont,
                                                 atoi(stringParts[1]), atoi(stringParts[2]), Front_MenuItem_Next,
                                                 autoCenter, menuSet->menus[atoi(stringParts[4] + 4) - 1], 1));
                } else if (*stringParts[4] != '\0') {
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_banner_item(
                                                 mission_replace_char(stringParts[3], '_', ' '), loFont, hiFont,
                                                 atoi(stringParts[1]), atoi(stringParts[2]), Front_MenuItem_Next,
                                                 autoCenter, menuSet->menus[atoi(stringParts[4] + 4) - 1], 0));
                } else {
                    /* The original passes the item type and the centring
                     * flag the other way round here. */
                    front_menu_add_menu_item(menuSet->menus[i],
                                             front_menu_item_create_banner_item(
                                                 mission_replace_char(stringParts[3], '_', ' '), loFont, hiFont,
                                                 atoi(stringParts[1]), atoi(stringParts[2]), autoCenter,
                                                 Front_MenuItem_Next, NULL, 0));
                }
                break;
            }
        }
        if (itemString != NULL) {
            free(itemString);
        }
    }
    return menuSet;
}

// FUNCTION: LEGORR 0x00414c90
int front_get_menu_id_by_name(Front_MenuSet* menuSet, const char* name)
{
    int i;

    for (i = 0; i < menuSet->menuCount; i++) {
        if (menuSet->menus[i] != NULL && _strcmpi(menuSet->menus[i]->title, name) == 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00414ce0
int front_is_intros_enabled(void)
{
    if (block_get_bool_path(legoGlobs_config,
                            // STRING: LEGORR 0x004a24e4
                            string_path_join(s_engine_config_root, "Main",
                                             // STRING: LEGORR 0x004a24ec
                                             "DontPlayAvis", NULL)) != 1 &&
        FRONT_PROGRAMMER_LEVEL < 3) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00414d30
void front_callback_slider_brightness(int brightness)
{
    float level;
    float spot;

    level = (float)brightness * 0.1f;
    spot = level * 0.8f - -0.2f;
    render_object_set_packed_rgba_from_float_channels(legoGlobs_spotlightTop, level, level, level, 1.0f);
    render_object_set_packed_rgba_from_float_channels((void*)(uintptr_t)legoGlobs_f_5560f8, spot, spot, spot, 1.0f);
    light_interface_call_slot_40(legoGlobs_spotlightTop, FRONT_FLOAT_ARG(level * 1.5f - -0.5f));
    light_interface_call_slot_3c(legoGlobs_spotlightTop, FRONT_FLOAT_ARG(level * 0.8f - -0.2f));
}

// FUNCTION: LEGORR 0x00414de0
void front_callback_slider_sound_volume(int volume)
{
    audio_sound_volume_index_set(volume);
}

/* ---- Part 2 (00414df0-00418190, finding 628) ---- */
/* Part 2's declarations follow part 1's functions, so that MSVC's symbol
 * counter at every part-1 function is the one part 1 was matched with
 * (finding 538, rule B). The original declared them all in FrontEnd.h. */
#include "gods98/files.h"
#include "gods98/movie_unit.h"

/* Front_Save_GetSaveDataAtIndex, Front_Save_GetCurrentSaveData and
 * Front_Save_GetRewardLevel take savegame.h's names in the matching build
 * (frontend_unit_link_names.h), and savegame.h, which part 1 already
 * includes, declares them returning void*; so they return void* here too,
 * and their users cast. The code is the same. */
#define FRONT_CURRENT_SAVE() ((Front_SaveData*)front_save_get_current_save_data())

/* 0x14 bytes (00417090): one level of a level set's link graph. */
typedef struct Front_LevelLink {
    int setIndex;                           /* +0x00 index in the set */
    struct Front_LevelLink** linkLevels;    /* +0x04 */
    unsigned int linkCount;                 /* +0x08 */
    int field_c;                            /* +0x0c */
    int visited;                            /* +0x10 */
} Front_LevelLink;

/* 0x14 bytes (004154b0): the missions or the tutorials. */
typedef struct Front_LevelSet {
    int count;                      /* +0x00 */
    char** idNames;                 /* +0x04 */
    char** langNames;               /* +0x08 */
    Front_LevelLink** levels;       /* +0x0c */
    int* visitedList;               /* +0x10 */
} Front_LevelSet;

/* One level's saved reward record, 0x3190 bytes (00417a90): a flag word
 * and Reward.c's 0x318c-byte level record. */
typedef struct Front_RewardLevel {
    unsigned char field_0[0x2fb4];  /* +0x0000 */
    float score;                    /* +0x2fb4 */
    unsigned char field_2fb8[0x1d4]; /* +0x2fb8 */
} Front_RewardLevel;

typedef struct Front_SaveReward {
    unsigned int flags;             /* +0x0000 1 completed, 2 a tutorial */
    Front_RewardLevel reward;       /* +0x0004 Reward.c's record */
} Front_SaveReward;

/* The six values a save keeps for Reward.c (00417f20). */
typedef struct Front_SaveStruct18 {
    int values[6];
} Front_SaveStruct18;

/* A save slot, 0xb8 bytes in frontGlobs (00417e50); the file holds the
 * first 0xb0 (read) or 0xb4 (written) bytes. */
typedef struct Front_SaveData {
    int field_0;                    /* +0x00 */
    int musicVolume;                /* +0x04 */
    int sfxVolume;                  /* +0x08 */
    int gameSpeed;                  /* +0x0c */
    int brightness;                 /* +0x10 */
    int field_14;                   /* +0x14 */
    unsigned int missionsCount;     /* +0x18 */
    Front_SaveStruct18 saveStruct18; /* +0x1c */
    Front_SaveReward* missionsTable; /* +0x34 */
    unsigned char field_38[0x80];   /* +0x38 */
} Front_SaveData;

/* Front_LevelLink_RunThroughLinks's callback (004171c0). */
typedef int (*Front_LevelLinkCallback)(Front_LevelLink* link, void* data);

/* Front_LevelLink_FindByLinkIndex's search (00417280). */
typedef struct Front_LevelLinkSearch {
    Front_LevelLink* resultLink;    /* +0x00 */
    unsigned int linkIndex;         /* +0x04 */
    unsigned int currentIndex;      /* +0x08 */
    unsigned int resultIndex;       /* +0x0c */
} Front_LevelLinkSearch;

/* Front_Levels_UpdateAvailable's state (004173e0). */
typedef struct Front_LevelUpdate {
    Front_SaveReward* rewards;      /* +0x00 */
    Front_LevelSet* levelSet;       /* +0x04 */
    Front_SelectData* select;       /* +0x08 */
    unsigned int index;             /* +0x0c */
    int setLocked;                  /* +0x10 */
} Front_LevelUpdate;

/* Front_Levels_LoadMenuItems's state (004179b0). */
typedef struct Front_LevelMenuBuild {
    Front_LevelSet* levelSet;       /* +0x00 */
    Front_Menu* menu;               /* +0x04 */
    Front_SelectData* select;       /* +0x08 */
    Front_SaveData* saveData;       /* +0x0c */
} Front_LevelMenuBuild;

#ifdef __cplusplus
extern "C" {
#endif
/* Part 2's callees (finding 628). */
extern float s_scroll_scale;                /* 0055615c legoGlobs.gameSpeed */
extern uint32_t legoGlobs_flags1;           /* 00556ecc (lego_unit.c) */
extern struct LevelState* g_active_level_retail; /* 005560d4 currLevel */
/* GAME-018 HelpWindow.c (helpwindow.c), GAME-019 Info.c (info_unit.c). */
void help_window_toggle(int toggle, int disable);                 /* 00419200 */
void info_set_auto_game_speed(int on);                            /* 00419450 */
/* GAME-021 Lego.c (lego_unit.c). */
void lego_transition_control(int toggle, unsigned int paused);    /* 00425c30 */
void lego_cd_audio_watchdog_mode_update(int on);                  /* 004297b0 */
void lego_audio_playback_toggle(int on);                          /* 00429810 */
int lego_level_load(const char* levelName);                       /* 00429890 */
const char* lego_level_free(void);                                /* 0042f0c0 */
/* GAME-029 Objective.c: Objective_SetStatus (objective_unit.cpp) and
 * Objective_IsEnded, whose port entry takes the level (lego_port.cpp's
 * original-ABI adapter). */
void objective_announce_current(int who);                         /* 00458670 */
int objective_is_ended_original(void);                            /* 004589a0 */
/* GAME-030 ObjectRecall.c: 00459360 has no port entry (an unannotated
 * adapter in frontend_port.cpp). */
void front_object_recall_restore_adapter(void);                   /* 00459360 */
/* gods98 Main.c (audio.cpp): Main_SetCDVolume, Main_GetCDVolume. */
void music_mixer_volume_set(float music, float sfx);              /* 00478980 */
int ledger_stub_004789a0_MusicMixerVolumeGet(float* music,
                                             float* sfx);         /* 004789a0 */
/* gods98 Utils.c (utils.c). */
char* string_duplicate(const char* text);                         /* 00477550 */
/* gods98 Input.c (input_unit.cpp): Input_AnyKeyPressed. */
int input_keyboard_down_key_count(void);                          /* 0047efb0 */
/* Container_MakeLight (004734f0): view.h types the colours as their
 * bits; the matching build passes the floats. Image_SaveBMP-style
 * adapters for ObjectRecall.c's save (004593e0), whose port entry takes
 * the game context: an unannotated adapter in frontend_port.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
void* ddiscene_light_create(void* parent, unsigned int type, float r, float g, float b); /* 004734f0 */
#else
void* ddiscene_light_create(void* owner, uint32_t value_a, uint32_t value_b,
                            uint32_t value_c, uint32_t value_d);
#endif
void* container_load(void* parent, const char* filename, const char* typeString,
                     int looseLoad);                                  /* 00472cd0 */
int sfx_random_set_and_play_global_sample(int sfxID, int* handle);   /* 00464ef0 */
void* text_buffer_create(void* font, float* area, unsigned int bufferSize); /* 004850f0 */
extern struct TextureSurface* s_lego_text_window_font;              /* 00556124 */
extern void* legoGlobs_fontBriefingLo;                              /* 00556138 */
void front_object_recall_save_adapter(const char* fname);           /* 004593e0 */
#ifdef __cplusplus
}
#endif

/* FrontEnd.c .bss: Front_Options_Update's menu and menu set. */
// GLOBAL: LEGORR 0x004db8e0
static Front_Menu* s_optionsMenu;
// GLOBAL: LEGORR 0x004db8e4
static Front_MenuSet* s_optionsMenuSet;
// GLOBAL: LEGORR 0x00556fc0
Front_MenuSet* frontGlobs_pausedMenuSet;
// GLOBAL: LEGORR 0x00556fc8
Front_MenuSet* frontGlobs_optionsMenuSet;
// GLOBAL: LEGORR 0x00556fd0
Front_LevelSet frontGlobs_tutorialLevels;
// GLOBAL: LEGORR 0x00556fe4
Front_LevelSet frontGlobs_missionLevels;
// GLOBAL: LEGORR 0x00556ff8
Front_LevelLink* g_front_save_tree;
// GLOBAL: LEGORR 0x00556ffc
Front_LevelLink* frontGlobs_startTutorialLink;
/* The main menu's item values, by menu (Front_RunScreenMenuType indexes
 * them): [0] the credits trigger, [3] quit, [4] the mission select, [5]
 * the tutorial select. */
// GLOBAL: LEGORR 0x00557010
int frontGlobs_mainMenuValues[6];
// GLOBAL: LEGORR 0x0055702c
int frontGlobs_triggerContinueMission;
// GLOBAL: LEGORR 0x00557030
int frontGlobs_sliderGameSpeed;
// GLOBAL: LEGORR 0x00557034
int frontGlobs_sliderSFXVolume;
// GLOBAL: LEGORR 0x00557038
int frontGlobs_sliderMusicVolume;
// GLOBAL: LEGORR 0x0055703c
int frontGlobs_sliderBrightness;
// GLOBAL: LEGORR 0x00557040
int frontGlobs_cycleHelpWindow;
// GLOBAL: LEGORR 0x00557044
int frontGlobs_triggerReplayObjective;
// GLOBAL: LEGORR 0x00557048
int frontGlobs_triggerQuitMission;
// GLOBAL: LEGORR 0x0055704c
int frontGlobs_triggerRestartMission;
// GLOBAL: LEGORR 0x00557050
int frontGlobs_cycleWallDetail;
// GLOBAL: LEGORR 0x00557054
int frontGlobs_cycleMusicOn;
// GLOBAL: LEGORR 0x00557058
int frontGlobs_cycleSFXOn;
// GLOBAL: LEGORR 0x0055705c
int frontGlobs_cycleAutoGameSpeed;
// GLOBAL: LEGORR 0x00557060
int frontGlobs_saveMenuSelect;
/* Five save slots and the current game's (slot 5). */
// GLOBAL: LEGORR 0x00557080
Front_SaveData frontGlobs_saveData[6];
// GLOBAL: LEGORR 0x005574d0
int frontGlobs_saveNumber;
// GLOBAL: LEGORR 0x005574d4
char* frontGlobs_defaultLevelBMPS;
// GLOBAL: LEGORR 0x00557500
int frontGlobs_isLoadSaveMenu;
// GLOBAL: LEGORR 0x00557554
char frontGlobs_langSaveGame[64];
// GLOBAL: LEGORR 0x0055781c
int frontGlobs_levelSelectUpdate;
// GLOBAL: LEGORR 0x00557824
int frontGlobs_levelSelectHoverNumber;
// GLOBAL: LEGORR 0x00557828
int frontGlobs_maxLevelScreens;
// GLOBAL: LEGORR 0x00557834
int frontGlobs_currLevelNumber;
// GLOBAL: LEGORR 0x00557838
int frontGlobs_prevLevelNumber;
// GLOBAL: LEGORR 0x0055783c
int frontGlobs_playLevelStream;
// GLOBAL: LEGORR 0x00557840
float frontGlobs_levelStreamTimer;

/* Part 2's functions. */
void front_callback_slider_music_volume(int volume); /* 00414df0 */
int front_get_music_volume_slider(void); /* 00414e10 */
void front_callback_cycle_wall_detail(int cycle); /* 00414e50 */
void front_callback_cycle_auto_game_speed(int cycle); /* 00414e80 */
void front_callback_cycle_music(int cycle); /* 00414ea0 */
void front_callback_cycle_sound(int cycle); /* 00414ec0 */
void front_callback_cycle_help(int cycle); /* 00414ee0 */
void front_callback_trigger_replay_objective(void); /* 00414f10 */
void front_callback_slider_game_speed(int slider); /* 00414f20 */
void front_update_game_speed_slider(void); /* 00414f90 */
int front_calc_slider_game_speed(void); /* 00415030 */
void front_callback_select_load_save(float elapsed, int index); /* 004150b0 */
void front_update_options_slider_levels(void); /* 00415150 */
void front_callback_trigger_back_save(void); /* 00415190 */
void front_load_selected_level(void); /* 00415220 */
void front_save_get_save_summary(Front_SaveData* saveData, char* buffer); /* 004152c0 */
void front_refresh_game_speed_slider(void); /* 00415360 */
int front_options_update(float elapsed, int modifyType); /* 00415370 */
int front_level_set_load_level_names(struct BlockRec* config, Front_LevelSet* levelSet, const char* startLevel); /* 004154b0 */
void front_play_movie(struct G98CMovie* movie, int skippable); /* 00415700 */
void front_play_intro_splash(const char* imageKey, int skippable, const char* timeKey); /* 004157c0 */
void front_play_intro_movie(const char* movieKey, int skippable); /* 00415910 */
void front_play_level_movie(const char* levelName, int skippable); /* 00415990 */
void front_load_levels(Front_MenuSet* menuSet); /* 00415a10 */
void front_save_clear_save_number(void); /* 00415cf0 */
void front_load_menu_text_window(struct BlockRec* config, const char* blockPath, Front_LevelTextWindow* menuWnd); /* 00415d00 */
int front_play_level_stream(int levelIndex); /* 00416150 */
int front_play_tutorial_stream(int levelIndex); /* 004161a0 */
void front_load_menus(struct BlockRec* config); /* 004161f0 */
void front_save_options_to_save_data(void); /* 00416910 */
void front_restore_options(int loadOptions, int resetMain); /* 00416940 */
void front_prepare_screen_menu_type(int menuType); /* 004169c0 */
int front_run_screen_menu_type(int menuType); /* 00416c80 */
int front_is_front_end_enabled(void); /* 00416d00 */
int front_is_mission_selected(void); /* 00416d50 */
int front_is_tutorial_selected(void); /* 00416d60 */
const char* front_get_selected_level(void); /* 00416d70 */
int front_is_quit_selected(void); /* 00416dd0 */
int front_is_quit_mission_selected(void); /* 00416de0 */
int front_is_restart_mission_selected(void); /* 00416df0 */
Front_LevelSet* front_get_current_level_set(void); /* 00416e00 */
int front_level_set_index_of(Front_LevelSet* levelSet, const char* levelName); /* 00416e20 */
void front_levels_reset_visited(void); /* 00416e70 */
void front_level_set_set_level_visited(Front_LevelSet* levelSet, const char* levelName, int visited); /* 00416ed0 */
int front_level_set_is_level_visited(Front_LevelSet* levelSet, const char* levelName); /* 00416f40 */
void front_level_set_set_level_link(Front_LevelSet* levelSet, const char* levelName, Front_LevelLink* link); /* 00416fb0 */
Front_LevelLink* front_level_set_get_level_link(Front_LevelSet* levelSet, const char* levelName); /* 00417020 */
Front_LevelLink* front_level_set_load_level_links(Front_LevelSet* levelSet, const char* levelName); /* 00417090 */
int front_level_link_run_through_links(Front_LevelLink* link, Front_LevelLinkCallback callback, void* data); /* 004171c0 */
int front_level_link_callback_inc_count(Front_LevelLink* link, void* data); /* 00417240 */
int front_level_link_callback_find_by_link_index(Front_LevelLink* link, void* data); /* 00417250 */
Front_LevelLink* front_level_link_find_by_link_index(Front_LevelLink* startLink, unsigned int linkIndex); /* 00417280 */
int front_level_link_get_set_index(Front_LevelLink* startLink, unsigned int linkIndex); /* 004172d0 */
void front_levels_update_available_recurse(Front_LevelLink* link, Front_LevelUpdate* update, int previousUnlocked); /* 004172f0 */
void front_levels_update_available(Front_LevelLink* startLink, Front_SaveReward* rewards, Front_LevelSet* levelSet, Front_SelectData* select, int setLocked); /* 004173e0 */
int front_save_get_level_score(unsigned int index, Front_SaveData* saveData); /* 00417430 */
void front_callback_select_mission_item(float elapsed, int selectIndex); /* 00417460 */
void front_callback_select_tutorial_item(float elapsed, int selectIndex); /* 00417700 */
int front_level_link_callback_load_menu_item(Front_LevelLink* link, void* data); /* 004177e0 */
void front_levels_load_menu_items(int menuType, Front_LevelLink* startLink, Front_LevelSet* levelSet, Front_Menu* menu, Front_SaveData* saveData, Front_Menu* nextMenu, Front_SelectCallback callback); /* 004179b0 */
int front_save_read_save_file(uint32_t saveIndex, void* outData, int readOnly); /* 00417a90 */
int front_save_write_save_file(unsigned int saveIndex, Front_SaveData* saveData); /* 00417bd0 */
void front_save_read_save_files(void); /* 00417df0 */
void* front_save_get_save_data_at_index(int saveIndex); /* 00417e50 */
void* front_save_get_current_save_data(void); /* 00417e70 */
int front_save_get_save_number(void); /* 00417e90 */
void front_save_set_save_number(int saveNumber); /* 00417ea0 */
void front_save_set_level_completed(unsigned int levelIndex); /* 00417eb0 */
void front_save_set_save_struct_18(const Front_SaveStruct18* saveStruct18); /* 00417f20 */
int front_save_set_reward_level(int levelIndex, const uint8_t* rewardLevel); /* 00417f40 */
void* front_save_get_reward_level(int levelIndex); /* 00417f90 */
void front_save_write_current_save_file(void); /* 00417fc0 */
int front_is_load_save_menu(void); /* 00417fe0 */
void front_set_load_save_menu(int isLoadSaveMenu); /* 00417ff0 */
void front_save_reset_current_save(void); /* 00418000 */
void front_save_copy_save_data(Front_SaveData* outData); /* 00418040 */
void front_save_restore_save_data(Front_SaveData* data); /* 004180c0 */
void front_levels_set_update(uint32_t update); /* 00418110 */
int front_save_is_game_100_percented(void); /* 00418120 */

// FUNCTION: LEGORR 0x00414df0
void front_callback_slider_music_volume(int volume)
{
    float level;

    level = (float)volume * 0.11111111f;
    music_mixer_volume_set(level, level);
}

// FUNCTION: LEGORR 0x00414e10
int front_get_music_volume_slider(void)
{
    float leftVolume;
    float rightVolume;
    float level;
    int volume;

    volume = 0;
    if (ledger_stub_004789a0_MusicMixerVolumeGet(&leftVolume, &rightVolume)) {
        level = (leftVolume + rightVolume) * 0.5f;
        volume = (int)(level * 9.0f);
    }
    return volume;
}

// FUNCTION: LEGORR 0x00414e50
void front_callback_cycle_wall_detail(int cycle)
{
    switch (cycle) {
    case 0:
        legoGlobs_flags1 |= 0x20;
        break;
    case 1:
        legoGlobs_flags1 &= ~0x20;
        break;
    }
}

// FUNCTION: LEGORR 0x00414e80
void front_callback_cycle_auto_game_speed(int cycle)
{
    info_set_auto_game_speed(cycle == 0);
}

// FUNCTION: LEGORR 0x00414ea0
void front_callback_cycle_music(int cycle)
{
    lego_cd_audio_watchdog_mode_update(cycle == 0);
}

// FUNCTION: LEGORR 0x00414ec0
void front_callback_cycle_sound(int cycle)
{
    lego_audio_playback_toggle(cycle == 0);
}

// FUNCTION: LEGORR 0x00414ee0
void front_callback_cycle_help(int cycle)
{
    switch (cycle) {
    case 0:
        help_window_toggle(0, 1);
        break;
    case 1:
        help_window_toggle(0, 0);
        break;
    }
}

// FUNCTION: LEGORR 0x00414f10
void front_callback_trigger_replay_objective(void)
{
    objective_announce_current(0);
}

// FUNCTION: LEGORR 0x00414f20
void front_callback_slider_game_speed(int slider)
{
    switch (slider) {
    case 0:
        s_scroll_scale = 0.33333334f;
        break;
    case 1:
        s_scroll_scale = 0.6666667f;
        break;
    case 3:
        s_scroll_scale = 1.3333334f;
        break;
    case 4:
        s_scroll_scale = 1.6666666f;
        break;
    case 5:
        s_scroll_scale = 2.0f;
        break;
    case 2:
    default:
        s_scroll_scale = 1.0f;
        break;
    }
}

// FUNCTION: LEGORR 0x00414f90
void front_update_game_speed_slider(void)
{
    if (s_scroll_scale < 0.66666667) {
        frontGlobs_sliderGameSpeed = 0;
    } else if (s_scroll_scale < 1.0) {
        frontGlobs_sliderGameSpeed = 1;
    } else if (s_scroll_scale < 1.33333333) {
        frontGlobs_sliderGameSpeed = 2;
    } else if (s_scroll_scale < 1.66666667) {
        frontGlobs_sliderGameSpeed = 3;
    } else {
        frontGlobs_sliderGameSpeed = 4;
        if (s_scroll_scale >= 2.0) {
            frontGlobs_sliderGameSpeed = 5;
        }
    }
}

// FUNCTION: LEGORR 0x00415030
int front_calc_slider_game_speed(void)
{
    if (s_scroll_scale < 0.66666667) {
        return 0;
    }
    if (s_scroll_scale < 1.0f) {
        return 1;
    }
    if (s_scroll_scale < 1.3333334f) {
        return 2;
    }
    if (s_scroll_scale < 1.6666666f) {
        return 3;
    }
    if (s_scroll_scale < 2.0f) {
        return 4;
    }
    return 5;
}

// FUNCTION: LEGORR 0x004150b0
void front_callback_select_load_save(float elapsed, int index)
{
    front_save_set_save_number(index);
    if (front_save_get_current_save_data() != NULL) {
        front_levels_update_available(g_front_save_tree,
            FRONT_CURRENT_SAVE()->missionsTable,
            &frontGlobs_missionLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData,
            0);
    } else {
        front_levels_update_available(g_front_save_tree, NULL,
            &frontGlobs_missionLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData,
            1);
    }
    front_levels_update_available(frontGlobs_startTutorialLink, NULL,
        &frontGlobs_tutorialLevels,
        (Front_SelectData*)frontGlobs_mainMenuSet->menus[2]->items[1]->itemData,
        0);
}

// FUNCTION: LEGORR 0x00415150
void front_update_options_slider_levels(void)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_save_data_at_index(front_save_get_save_number());
    if (saveData != NULL) {
        front_callback_slider_sound_volume(saveData->sfxVolume);
        front_callback_slider_brightness(saveData->brightness);
        front_callback_slider_music_volume(saveData->musicVolume);
    }
}

// FUNCTION: LEGORR 0x00415190
void front_callback_trigger_back_save(void)
{
    g_front_save_menu_result = 0;
    g_front_save_menu_open = 0;
    s_selectClickIndex = -1;
    if (front_save_get_current_save_data() != NULL) {
        front_levels_update_available(g_front_save_tree,
            FRONT_CURRENT_SAVE()->missionsTable,
            &frontGlobs_missionLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData,
            0);
    } else {
        front_levels_update_available(g_front_save_tree, NULL,
            &frontGlobs_missionLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData,
            1);
    }
}

// FUNCTION: LEGORR 0x00415220
void front_load_selected_level(void)
{
    char buffer[128];
    int i;

    sprintf(buffer, "%s", *(const char**)g_active_level_retail);
    lego_transition_control(0, 0);
    lego_level_free();
    lego_level_load(string_duplicate(buffer));
    for (i = 0; i < frontGlobs_pausedMenuSet->menuCount; i++) {
        if (frontGlobs_pausedMenuSet->menus[i] != NULL) {
            frontGlobs_pausedMenuSet->menus[i]->closed = 1;
        }
    }
    frontGlobs_triggerRestartMission = 0;
    legoGlobs_flags1 |= 0x2000000;
    front_object_recall_restore_adapter();
}

// FUNCTION: LEGORR 0x004152c0
void front_save_get_save_summary(Front_SaveData* saveData, char* buffer)
{
    unsigned int i;
    int totalScore;
    unsigned int completed;

    if (saveData != NULL) {
        totalScore = 0;
        completed = 0;
        for (i = 0; i < saveData->missionsCount; i++) {
            totalScore += front_save_get_level_score(i, saveData);
            if (saveData->missionsTable[i].flags & 1) {
                completed++;
            }
        }
        if (completed != 0) {
            // STRING: LEGORR 0x004a24fc
            sprintf(buffer, "Game %i%% complete with %i points scored",
                    (int)((float)completed / saveData->missionsCount * 100.0f),
                    totalScore);
        }
    }
}

// FUNCTION: LEGORR 0x00415360
void front_refresh_game_speed_slider(void)
{
    frontGlobs_sliderGameSpeed = front_calc_slider_game_speed();
}

// FUNCTION: LEGORR 0x00415370
int front_options_update(float elapsed, int modifyType)
{
    Front_MenuSet* menuSet;
    int result;

    result = 1;
    front_refresh_game_speed_slider();
    if (modifyType == 0) {
        menuSet = frontGlobs_pausedMenuSet;
    } else if (modifyType == 1) {
        menuSet = frontGlobs_optionsMenuSet;
    } else {
        return 1;
    }

    if (s_optionsMenu == NULL || frontGlobs_triggerRestartMission != 0 ||
        frontGlobs_triggerQuitMission != 0 || frontGlobs_triggerReplayObjective != 0 ||
        s_optionsMenuSet != menuSet || objective_is_ended_original()) {
        frontGlobs_triggerQuitMission = 0;
        s_optionsMenu = menuSet->menus[0];
        s_optionsMenuSet = menuSet;
        if (frontGlobs_triggerReplayObjective != 0) {
            if (menuSet->menus[0] != NULL) {
                menuSet->menus[0]->closed = 1;
            }
            if (menuSet->menus[2] != NULL) {
                menuSet->menus[2]->closed = 1;
            }
            if (modifyType == 0) {
                frontGlobs_triggerReplayObjective = 0;
                return 0;
            }
            return 0;
        }
        if (frontGlobs_triggerRestartMission == 1) {
            return 0;
        }
    }

    if (!objective_is_ended_original()) {
        s_optionsMenu = front_menu_update(elapsed, s_optionsMenu, NULL);
    }
    if (modifyType == 0) {
        result = 1;
        if (menuSet->menus[0]->closed != 1 && menuSet->menus[2]->closed != 1) {
            result = 0;
        }
        menuSet->menus[0]->closed = 0;
        menuSet->menus[2]->closed = 0;
    } else if (modifyType == 1) {
        result = (menuSet->menus[0]->closed == 1);
        menuSet->menus[0]->closed = 0;
    }
    return result;
}

// FUNCTION: LEGORR 0x004154b0
int front_level_set_load_level_names(struct BlockRec* config, Front_LevelSet* levelSet,
                                     const char* startLevel)
{
    const char* levelName;
    int count;
    int i;

    levelName = block_get_str_copy(legoGlobs_config,
                                   string_path_join(s_engine_config_root, "Main", startLevel, NULL));
    if (levelName == NULL) {
        return 0;
    }
    count = 1;
    levelName = block_get_str_copy(legoGlobs_config,
                                   // STRING: LEGORR 0x004a2528
                                   string_path_join(s_engine_config_root, levelName, "NextLevel", NULL));
    while (levelName != NULL) {
        count++;
        levelName = block_get_str_copy(legoGlobs_config,
                                       string_path_join(s_engine_config_root, levelName, "NextLevel", NULL));
    }

    levelSet->count = count;
    levelSet->idNames = (char**)malloc(count * sizeof(char*));
    levelSet->langNames = (char**)malloc(count * sizeof(char*));
    levelSet->levels = (Front_LevelLink**)malloc(count * sizeof(Front_LevelLink*));
    memset(levelSet->levels, 0, count * sizeof(Front_LevelLink*));
    levelSet->visitedList = (int*)malloc(count * sizeof(int));
    memset(levelSet->visitedList, 0, count * sizeof(int));

    if (levelSet->idNames == NULL) {
        return 0;
    }
    if (levelSet->langNames == NULL) {
        return 0;
    }

    levelSet->idNames[0] = block_get_str_copy(legoGlobs_config,
                                              string_path_join(s_engine_config_root, "Main", startLevel, NULL));
    levelName = levelSet->idNames[0];
    levelSet->langNames[0] = block_get_str_copy(legoGlobs_config,
                                                string_path_join(s_engine_config_root, levelName, "FullName", NULL));
    if (levelSet->langNames[0] == NULL) {
        levelSet->langNames[0] = front_util_string_copy(levelSet->idNames[0]);
    } else {
        mission_replace_char(levelSet->langNames[0], '_', ' ');
    }

    for (i = 1; i < levelSet->count; i++) {
        levelSet->idNames[i] = block_get_str_copy(legoGlobs_config,
                                                  string_path_join(s_engine_config_root, levelName, "NextLevel", NULL));
        levelName = levelSet->idNames[i];
        levelSet->langNames[i] = block_get_str_copy(legoGlobs_config,
                                                    string_path_join(s_engine_config_root, levelName, "FullName", NULL));
        if (levelSet->langNames[i] == NULL) {
            levelSet->langNames[i] = front_util_string_copy(levelSet->idNames[i]);
        } else {
            mission_replace_char(levelSet->langNames[i], '_', ' ');
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00415700
void front_play_movie(struct G98CMovie* movie, int skippable)
{
    float timer;
    RECT rect;

    rect.left = 0;
    rect.top = 0;
    rect.right = 640;
    rect.bottom = 480;
    timer = 0.0f;
    if (movie_update(movie, 1.0f, (struct tagRECT*)&rect)) {
        while (1) {
            main_loop_frame_pump(0);
            timer = timer - -1.0f;
            if ((float)movie_get_duration(movie) < timer) {
                return;
            }
            if (skippable) {
                if (input_keyboard_down_key_count()) {
                    return;
                }
                if (s_window_mouse_word_40) {
                    return;
                }
                if (g_renderer_word_c) {
                    return;
                }
            }
            if (!movie_update(movie, 1.0f, (struct tagRECT*)&rect)) {
                return;
            }
        }
    }
}

// FUNCTION: LEGORR 0x004157c0
void front_play_intro_splash(const char* imageKey, int skippable, const char* timeKey)
{
    float seconds;
    unsigned int endTime;
    char* imageName;
    struct Image* image;

    seconds = (float)atof(block_get_strptr(legoGlobs_config,
                                           string_path_join(s_engine_config_root, "Main", timeKey, NULL))
                              ? block_get_strptr(legoGlobs_config,
                                                 string_path_join(s_engine_config_root, "Main", timeKey, NULL))
                              : s_config_angle_empty);
    if (0.0f == seconds) {
        seconds = 3.0f;
    }
    endTime = timeGetTime() - (int)(seconds * -1000.0f);
    if (imageKey != NULL) {
        imageName = block_get_str_copy(legoGlobs_config,
                                       string_path_join(s_engine_config_root, "Main", imageKey, NULL));
        if (imageName != NULL && (image = (struct Image*)image_load_bmp_scaled(imageName, 0, 0)) != NULL) {
            {
                while (1) {
                    image_display_scaled((Image*)image, NULL, NULL, NULL);
                    main_loop_frame_pump(0);
                    Sleep(100);
                    if (timeGetTime() >= endTime) {
                        break;
                    }
                    if (skippable && (input_keyboard_down_key_count() || s_window_mouse_word_40 ||
                                      g_renderer_word_c)) {
                        break;
                    }
                }
                image_surface_release(image);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00415910
void front_play_intro_movie(const char* movieKey, int skippable)
{
    char* movieName;
    struct G98CMovie* movie;

    if (movieKey != NULL) {
        movieName = block_get_str_copy(legoGlobs_config,
                                       string_path_join(s_engine_config_root, "Main", movieKey, NULL));
        if (movieName != NULL && file_exists(movieName)) {
            movie = movie_load(movieName);
            front_play_movie(movie, skippable);
            movie_free(movie);
            free(movieName);
        }
    }
}

// FUNCTION: LEGORR 0x00415990
void front_play_level_movie(const char* levelName, int skippable)
{
    char* movieName;
    struct G98CMovie* movie;

    if (levelName != NULL) {
        movieName = block_get_str_copy(legoGlobs_config,
                                       // STRING: LEGORR 0x004a2534
                                       string_path_join(s_engine_config_root, levelName, "Video", NULL));
        if (movieName != NULL) {
            if (file_exists(movieName)) {
                movie = movie_load(movieName);
                front_play_movie(movie, skippable);
                movie_free(movie);
            }
            free(movieName);
        }
    }
}

// FUNCTION: LEGORR 0x00415a10
void front_load_levels(Front_MenuSet* menuSet)
{
    int levelsIndex;
    int tutorialsIndex;
    int loadSaveIndex;
    Front_SelectData* loadSelect;
    Front_SelectData* saveSelect;
    Front_SaveData* saveData;
    int i;
    char buffer[100];

    g_front_save_tree = front_level_set_load_level_links(&frontGlobs_missionLevels,
                                                                   frontGlobs_missionLevels.idNames[0]);
    frontGlobs_startTutorialLink = front_level_set_load_level_links(&frontGlobs_tutorialLevels,
                                                                    frontGlobs_tutorialLevels.idNames[0]);
    front_levels_reset_visited();

    levelsIndex = front_get_menu_id_by_name(frontGlobs_mainMenuSet, "Levels");
    // STRING: LEGORR 0x004a2568
    tutorialsIndex = front_get_menu_id_by_name(frontGlobs_mainMenuSet, "Tutorials");
    // STRING: LEGORR 0x004a2558
    loadSaveIndex = front_get_menu_id_by_name(frontGlobs_mainMenuSet, "Load Level Save");

    loadSelect = front_menu_item_create_select(&frontGlobs_f_557028, s_config_angle_empty,
                                               s_config_angle_empty, -200, -100, 60, 5, -20, 200,
                                               -20, -40, 0, front_callback_select_load_save,
                                               frontGlobs_mainMenuSet->menus[levelsIndex]);
    front_menu_add_menu_item(frontGlobs_mainMenuSet->menus[loadSaveIndex],
        front_menu_item_create_banner_item(s_config_angle_empty,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontLo,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontHi,
            0, 0, Front_MenuItem_Select, 1, loadSelect, 0));

    saveSelect = front_menu_item_create_select(&frontGlobs_f_557028, s_config_angle_empty,
                                               s_config_angle_empty, -200, -100, 60, 5, -20, 200,
                                               -20, -40, 0, front_callback_select_load_save, NULL);
    front_menu_add_menu_item(frontGlobs_saveMenuSet->menus[0],
        front_menu_item_create_banner_item(s_config_angle_empty,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontLo,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontHi,
            0, 0, Front_MenuItem_Select, 1, saveSelect, 0));

    for (i = 0; i < 5; i++) {
        saveData = &frontGlobs_saveData[i];
        // STRING: LEGORR 0x004a253c
        sprintf(buffer, "%s %i",
                block_get_strptr(legoGlobs_config,
                                 string_path_join(s_engine_config_root,
                                                  // STRING: LEGORR 0x004a2544
                                                  "Menu",
                                                  // STRING: LEGORR 0x004a254c
                                                  "Save_Game", NULL)),
                i + 1);
        mission_replace_char(buffer, '_', ' ');
        front_save_get_save_summary(saveData, buffer);
        front_menu_item_add_select_item(loadSelect, buffer, 1,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontHi, 0, 0, 0);
        front_menu_item_add_select_item(saveSelect, buffer, 1,
            frontGlobs_mainMenuSet->menus[loadSaveIndex]->items[0]->fontHi, 0, 0, 0);
    }

    front_save_read_save_files();
    front_levels_load_menu_items(4, g_front_save_tree, &frontGlobs_missionLevels,
                                 frontGlobs_mainMenuSet->menus[levelsIndex],
                                 FRONT_CURRENT_SAVE(), NULL,
                                 front_callback_select_mission_item);
    front_levels_load_menu_items(5, frontGlobs_startTutorialLink, &frontGlobs_tutorialLevels,
                                 frontGlobs_mainMenuSet->menus[tutorialsIndex],
                                 FRONT_CURRENT_SAVE(), NULL,
                                 front_callback_select_tutorial_item);
}

// FUNCTION: LEGORR 0x00415cf0
void front_save_clear_save_number(void)
{
    frontGlobs_saveNumber = -1;
}

// FUNCTION: LEGORR 0x00415d00
void front_load_menu_text_window(struct BlockRec* config, const char* blockPath,
                                 Front_LevelTextWindow* menuWnd)
{
    char* str;
    char* stringParts[5];
    float area[4] = { 0 };

    // STRING: LEGORR 0x004a25c0
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Load", NULL));
    if (str != NULL) {
        strcpy(menuWnd->loadText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a25b8
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Save", NULL));
    if (str != NULL) {
        strcpy(menuWnd->saveText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a25b0
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Slot", NULL));
    if (str != NULL) {
        strcpy(menuWnd->slotText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a25a8
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "SaveSel", NULL));
    if (str != NULL) {
        strcpy(menuWnd->saveSelText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a25a0
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "LoadSel", NULL));
    if (str != NULL) {
        strcpy(menuWnd->loadSelText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a2598
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Level", NULL));
    if (str != NULL) {
        strcpy(menuWnd->levelText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a258c
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Tutorial", NULL));
    if (str != NULL) {
        strcpy(menuWnd->tutorialText, front_util_replace_text_spaces(str));
    }
    // STRING: LEGORR 0x004a2584
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Window", NULL));
    if (str != NULL && sound_variant_split(str, stringParts, "|") == 4) {
        menuWnd->window[0] = area[0] = (float)atoi(stringParts[0]);
        menuWnd->window[1] = area[1] = (float)atoi(stringParts[1]);
        menuWnd->window[2] = area[2] = (float)atoi(stringParts[2]);
        menuWnd->window[3] = area[3] = (float)atoi(stringParts[3]);
        menuWnd->textWindow = text_buffer_create(s_lego_text_window_font, area, 0x200);
    }
    // STRING: LEGORR 0x004a257c
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, blockPath, "Panel", NULL));
    if (str != NULL && sound_variant_split(str, stringParts, "|") == 5 && stringParts[0] != NULL) {
        menuWnd->panelImage = (struct Image*)image_load_bmp_scaled(stringParts[0], 0, 0);
        if (menuWnd->panelImage != NULL) {
            menuWnd->panelPos[0] = (float)atoi(stringParts[1]);
            menuWnd->panelPos[1] = (float)atoi(stringParts[2]);
            menuWnd->panelSize[0] = (float)atoi(stringParts[3]);
            menuWnd->panelSize[1] = (float)atoi(stringParts[4]);
            image_surface_blit_transformed((struct TextureSurface*)menuWnd->panelImage,
                                           0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
}

// FUNCTION: LEGORR 0x00416150
int front_play_level_stream(int levelIndex)
{
    int sfxID;
    char buffer[128];

    // STRING: LEGORR 0x004a25c8
    sprintf(buffer, "Stream_LevelName_Level%d", levelIndex);
    if (sfx_get_type(buffer, &sfxID)) {
        return sfx_random_set_and_play_global_sample(sfxID, NULL);
    }
#ifdef __cplusplus
    return 0;
#endif
    /* (C) No return: the original leaves SFX_GetType's 0 in eax. */
}

// FUNCTION: LEGORR 0x004161a0
int front_play_tutorial_stream(int levelIndex)
{
    int sfxID;
    char buffer[128];

    // STRING: LEGORR 0x004a25e4
    sprintf(buffer, "Stream_LevelName_TLevel%d", levelIndex);
    if (sfx_get_type(buffer, &sfxID)) {
        return sfx_random_set_and_play_global_sample(sfxID, NULL);
    }
#ifdef __cplusplus
    return 0;
#endif
    /* (C) No return: the original leaves SFX_GetType's 0 in eax. */
}

// FUNCTION: LEGORR 0x004161f0
void front_load_menus(struct BlockRec* config)
{
    char* str;

    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu",
                                                                                              // STRING: LEGORR 0x004a26d0
                                                                                              "MenuWipe", NULL));
    if (str != NULL) {
        if (strnicmp(str, s_overlayNullName, 4) != 0) {
            frontGlobs_rockWipeAnim = container_load(NULL, str, "LWS", 1);
        }
        free(str);
    } else {
        str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu",
                                                                                                  // STRING: LEGORR 0x004a26c4
                                                                                                  "!MenuWipe", NULL));
        if (str != NULL) {
            if (!(s_engine_feature_word & 0x400000) && strnicmp(str, s_overlayNullName, 4) != 0) {
                frontGlobs_rockWipeAnim = container_load(NULL, str, "LWS", 1);
            }
            free(str);
        }
    }

    frontGlobs_saveTextWnd = (Front_SaveTextWindow*)malloc(sizeof(Front_LevelTextWindow));
    memset(frontGlobs_saveTextWnd, 0, sizeof(Front_LevelTextWindow));
    // STRING: LEGORR 0x004a1f10
    front_load_menu_text_window(config, "Menu::SaveText", (Front_LevelTextWindow*)frontGlobs_saveTextWnd);
    frontGlobs_saveLevelWnd = (Front_LevelTextWindow*)malloc(sizeof(Front_LevelTextWindow));
    memset(frontGlobs_saveLevelWnd, 0, sizeof(Front_LevelTextWindow));
    // STRING: LEGORR 0x004a1f20
    front_load_menu_text_window(config, "Menu::LevelText", frontGlobs_saveLevelWnd);

    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu", "Save_Game", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_langSaveGame, front_util_replace_text_spaces(str));
        free(str);
    }
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu",
                                                                                              // STRING: LEGORR 0x004a26b8
                                                                                              "Load_Game", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_saveSlotFormat, front_util_replace_text_spaces(str));
        free(str);
    }
    // STRING: LEGORR 0x004a1f30
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu::Overwrite", "Title", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_overwriteTitle, front_util_replace_text_spaces(str));
        free(str);
    }
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu::Overwrite",
                                                                                                         // STRING: LEGORR 0x004a26b0
                                                                                                         "Text", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_overwriteText, front_util_replace_text_spaces(str));
        free(str);
    }
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu::Overwrite",
                                                                                                         // STRING: LEGORR 0x004a26ac
                                                                                                         "Ok", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_overwriteOK, front_util_replace_text_spaces(str));
    }
    str = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, "Menu::Overwrite",
                                                                                                         // STRING: LEGORR 0x004a26a4
                                                                                                         "Cancel", NULL));
    if (str != NULL) {
        strcpy(frontGlobs_overwriteCancel, front_util_replace_text_spaces(str));
    }

    frontGlobs_versionFont = (Front_Font*)legoGlobs_fontBriefingLo;
    frontGlobs_versionString = block_get_str_copy(legoGlobs_config,
                                                  string_path_join(s_engine_config_root, "Main",
                                                                                                 // STRING: LEGORR 0x004a269c
                                                                                                 "Version", NULL));
    frontGlobs_defaultLevelBMPS = block_get_str_copy(legoGlobs_config,
        string_path_join(s_engine_config_root, "Menu",
                                                       // STRING: LEGORR 0x004a2688
                                                       "DefaultLevelBMPS", NULL));

    frontGlobs_rockWipeLight = ddiscene_light_create(legoGlobs_rootCont, 3, FRONT_FLOAT_ARG(0.8f),
                                                     FRONT_FLOAT_ARG(0.8f), FRONT_FLOAT_ARG(0.8f));
    scene_render_lock_toggle(frontGlobs_rockWipeLight, 1);

    // STRING: LEGORR 0x004a2678
    g_front_main_menu_set = front_load_menu_set(config, "MainMenuFull",
                                                &frontGlobs_mainMenuValues[0], front_callback_trigger_play_credits,
                                                &frontGlobs_mainMenuValues[3], NULL, -1);
    // STRING: LEGORR 0x004a266c
    frontGlobs_saveMenuSet = front_load_menu_set(config, "SaveMenu", &frontGlobs_saveMenuSelect,
                                                 front_callback_trigger_back_save, -1);
    // STRING: LEGORR 0x004a2660
    front_level_set_load_level_names(legoGlobs_config, &frontGlobs_missionLevels, "StartLevel");
    // STRING: LEGORR 0x004a264c
    front_level_set_load_level_names(legoGlobs_config, &frontGlobs_tutorialLevels, "TutorialStartLevel");
    front_load_levels(frontGlobs_mainMenuSet);

    // STRING: LEGORR 0x004a2640
    frontGlobs_pausedMenuSet = front_load_menu_set(config, "PausedMenu",
        &frontGlobs_triggerContinueMission, NULL,
        &frontGlobs_sliderGameSpeed, front_callback_slider_game_speed,
        &frontGlobs_sliderSFXVolume, front_callback_slider_sound_volume,
        &frontGlobs_sliderMusicVolume, front_callback_slider_music_volume,
        &frontGlobs_sliderBrightness, front_callback_slider_brightness,
        &frontGlobs_cycleHelpWindow, front_callback_cycle_help,
        &frontGlobs_triggerReplayObjective, front_callback_trigger_replay_objective,
        &frontGlobs_triggerQuitMission, NULL,
        &frontGlobs_triggerRestartMission, NULL,
        &frontGlobs_cycleWallDetail, front_callback_cycle_wall_detail,
        &frontGlobs_cycleMusicOn, front_callback_cycle_music,
        &frontGlobs_cycleSFXOn, front_callback_cycle_sound,
        &frontGlobs_cycleAutoGameSpeed, front_callback_cycle_auto_game_speed,
        -1);
    // STRING: LEGORR 0x004a2634
    frontGlobs_optionsMenuSet = front_load_menu_set(config, "OptionsMenu",
        &frontGlobs_sliderGameSpeed, front_callback_slider_game_speed,
        &frontGlobs_sliderSFXVolume, front_callback_slider_sound_volume,
        &frontGlobs_sliderMusicVolume, front_callback_slider_music_volume,
        &frontGlobs_sliderBrightness, front_callback_slider_brightness,
        &frontGlobs_cycleHelpWindow, front_callback_cycle_help,
        &frontGlobs_triggerReplayObjective, front_callback_trigger_replay_objective,
        &frontGlobs_triggerContinueMission, NULL,
        -1);

    frontGlobs_sliderSFXVolume = 8;
    frontGlobs_triggerContinueMission = 0;
    frontGlobs_sliderBrightness = 5;
    frontGlobs_cycleHelpWindow = 1;
    frontGlobs_triggerReplayObjective = 0;
    frontGlobs_sliderGameSpeed = front_calc_slider_game_speed();
    frontGlobs_sliderMusicVolume = front_get_music_volume_slider();
    frontGlobs_isLoadSaveMenu = 0;

    frontGlobs_maxLevelScreens = atoi(
        block_get_strptr(config, string_path_join(s_engine_config_root, "Menu",
                                                                                // STRING: LEGORR 0x004a2624
                                                                                "MaxLevelScreens", NULL))
            ? block_get_strptr(config, string_path_join(s_engine_config_root, "Menu", "MaxLevelScreens", NULL))
            : s_config_angle_empty);
    if (frontGlobs_maxLevelScreens == 0) {
        frontGlobs_maxLevelScreens = 1;
    }
    frontGlobs_levelSelectHoverNumber = 0;

    frontGlobs_saveImageBigWidth = atoi(
        block_get_strptr(config, string_path_join(s_engine_config_root, "Menu",
                                                  // STRING: LEGORR 0x004a260c
                                                  "SaveImage",
                                                  // STRING: LEGORR 0x004a2618
                                                  "BigWidth", NULL))
            ? block_get_strptr(config, string_path_join(s_engine_config_root, "Menu", "SaveImage", "BigWidth", NULL))
            : s_config_angle_empty);
    if (frontGlobs_saveImageBigWidth == 0) {
        frontGlobs_saveImageBigWidth = 80;
    }
    frontGlobs_saveImageBigHeight = atoi(
        block_get_strptr(config, string_path_join(s_engine_config_root, "Menu", "SaveImage",
                                                                                             // STRING: LEGORR 0x004a2600
                                                                                             "BigHeight", NULL))
            ? block_get_strptr(config, string_path_join(s_engine_config_root, "Menu", "SaveImage", "BigHeight", NULL))
            : s_config_angle_empty);
    if (frontGlobs_saveImageBigHeight == 0) {
        frontGlobs_saveImageBigHeight = 60;
    }
    front_levels_set_update(1);
}

// FUNCTION: LEGORR 0x00416910
void front_save_options_to_save_data(void)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_current_save_data();
    if (saveData != NULL) {
        saveData->gameSpeed = frontGlobs_sliderGameSpeed;
        saveData->sfxVolume = frontGlobs_sliderSFXVolume;
        saveData->brightness = frontGlobs_sliderBrightness;
    }
}

// FUNCTION: LEGORR 0x00416940
void front_restore_options(int loadOptions, int resetMain)
{
    Front_SaveData* saveData;

    if (resetMain) {
        frontGlobs_mainMenuValues[3] = 0;
        frontGlobs_mainMenuValues[4] = -1;
        frontGlobs_mainMenuValues[5] = -1;
        frontGlobs_triggerContinueMission = 0;
    }
    if (loadOptions) {
        if (frontGlobs_f_557028 != -1 && front_save_get_current_save_data() != NULL) {
            saveData = FRONT_CURRENT_SAVE();
            frontGlobs_sliderGameSpeed = saveData->gameSpeed;
            frontGlobs_sliderSFXVolume = saveData->sfxVolume;
            frontGlobs_sliderBrightness = saveData->brightness;
            return;
        }
        frontGlobs_sliderGameSpeed = 1;
        frontGlobs_sliderSFXVolume = 8;
        frontGlobs_sliderBrightness = 5;
    }
}

// FUNCTION: LEGORR 0x004169c0
void front_prepare_screen_menu_type(int menuType)
{
    switch (menuType) {
    case 0:
        frontGlobs_f_557028 = -1;
        front_restore_options(1, 1);
        if (front_save_get_current_save_data() != NULL) {
            front_levels_update_available(g_front_save_tree,
                FRONT_CURRENT_SAVE()->missionsTable, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 0);
        } else if (frontGlobs_levelSelectUpdate) {
            front_levels_update_available(g_front_save_tree, NULL, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 1);
        }
        front_levels_update_available(frontGlobs_startTutorialLink, NULL, &frontGlobs_tutorialLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[2]->items[1]->itemData, 0);
        break;
    case 1:
        front_restore_options(1, 1);
        if (front_save_get_current_save_data() != NULL) {
            front_levels_update_available(g_front_save_tree,
                FRONT_CURRENT_SAVE()->missionsTable, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 0);
        } else if (frontGlobs_levelSelectUpdate) {
            front_levels_update_available(g_front_save_tree, NULL, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 1);
        }
        front_levels_update_available(frontGlobs_startTutorialLink, NULL, &frontGlobs_tutorialLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[2]->items[1]->itemData, 0);
        break;
    case 2:
        front_restore_options(1, 1);
        if (front_save_get_current_save_data() != NULL) {
            front_levels_update_available(g_front_save_tree,
                FRONT_CURRENT_SAVE()->missionsTable, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 0);
        } else if (frontGlobs_levelSelectUpdate) {
            front_levels_update_available(g_front_save_tree, NULL, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 1);
        }
        front_levels_update_available(frontGlobs_startTutorialLink, NULL, &frontGlobs_tutorialLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[2]->items[1]->itemData, 0);
        break;
    case 3:
        front_restore_options(0, 1);
        break;
    case 4:
        front_restore_options(0, 1);
        if (front_save_get_current_save_data() != NULL) {
            front_levels_update_available(g_front_save_tree,
                FRONT_CURRENT_SAVE()->missionsTable, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 0);
        } else if (frontGlobs_levelSelectUpdate) {
            front_levels_update_available(g_front_save_tree, NULL, &frontGlobs_missionLevels,
                (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 1);
            frontGlobs_isLoadSaveMenu = 1;
        }
        front_levels_update_available(frontGlobs_startTutorialLink, NULL, &frontGlobs_tutorialLevels,
            (Front_SelectData*)frontGlobs_mainMenuSet->menus[2]->items[1]->itemData, 0);
        break;
    }
    front_levels_set_update(1);
}

// FUNCTION: LEGORR 0x00416c80
int front_run_screen_menu_type(int menuType)
{
    Front_MenuSet* menuSet;

    menuSet = frontGlobs_mainMenuSet;
    front_prepare_screen_menu_type(menuType);
    sfx_add_to_queue(0x2c, 1);
    switch (menuType) {
    case 0:
        front_screen_menu_loop(menuSet, 0);
        break;
    case 1:
        front_screen_menu_loop(menuSet, 1);
        break;
    case 2:
        front_screen_menu_loop(menuSet, 2);
        break;
    case 4:
        front_screen_menu_loop(frontGlobs_saveMenuSet, 0);
        break;
    }
    sound_stream_stop_and_release(1);
    sound_stream_stop_and_release(0);
    return 0;
}

// FUNCTION: LEGORR 0x00416d00
int front_is_front_end_enabled(void)
{
    if (block_get_bool_path(legoGlobs_config, string_path_join(s_engine_config_root, "Main",
                                                                                             // STRING: LEGORR 0x004a26dc
                                                                                             "FrontEnd", NULL)) == 1 &&
        FRONT_PROGRAMMER_LEVEL < 3) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00416d50
int front_is_mission_selected(void)
{
    return frontGlobs_mainMenuValues[4] != -1;
}

// FUNCTION: LEGORR 0x00416d60
int front_is_tutorial_selected(void)
{
    return frontGlobs_mainMenuValues[5] != -1;
}

// FUNCTION: LEGORR 0x00416d70
const char* front_get_selected_level(void)
{
    int index;

    if (front_is_mission_selected()) {
        index = front_level_link_get_set_index(g_front_save_tree, frontGlobs_mainMenuValues[4]);
        if (index != -1) {
            return frontGlobs_missionLevels.idNames[index];
        }
    } else if (front_is_tutorial_selected()) {
        index = front_level_link_get_set_index(frontGlobs_startTutorialLink, frontGlobs_mainMenuValues[5]);
        if (index != -1) {
            return frontGlobs_tutorialLevels.idNames[index];
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00416dd0
int front_is_quit_selected(void)
{
    return frontGlobs_mainMenuValues[3];
}

// FUNCTION: LEGORR 0x00416de0
int front_is_quit_mission_selected(void)
{
    return frontGlobs_triggerQuitMission != 0;
}

// FUNCTION: LEGORR 0x00416df0
int front_is_restart_mission_selected(void)
{
    return frontGlobs_triggerRestartMission != 0;
}

// FUNCTION: LEGORR 0x00416e00
Front_LevelSet* front_get_current_level_set(void)
{
    return front_is_tutorial_selected() ? &frontGlobs_tutorialLevels : &frontGlobs_missionLevels;
}

// FUNCTION: LEGORR 0x00416e20
int front_level_set_index_of(Front_LevelSet* levelSet, const char* levelName)
{
    int i;

    for (i = 0; i < levelSet->count; i++) {
        if (_strcmpi(levelName, levelSet->idNames[i]) == 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00416e70
void front_levels_reset_visited(void)
{
    int i;

    for (i = 0; i < frontGlobs_missionLevels.count; i++) {
        if (frontGlobs_missionLevels.levels[i] != NULL) {
            frontGlobs_missionLevels.levels[i]->visited = 0;
            frontGlobs_missionLevels.visitedList[i] = 0;
        }
    }
    for (i = 0; i < frontGlobs_tutorialLevels.count; i++) {
        if (frontGlobs_tutorialLevels.levels[i] != NULL) {
            frontGlobs_tutorialLevels.levels[i]->visited = 0;
            frontGlobs_tutorialLevels.visitedList[i] = 0;
        }
    }
}

// FUNCTION: LEGORR 0x00416ed0
void front_level_set_set_level_visited(Front_LevelSet* levelSet, const char* levelName, int visited)
{
    int i;

    for (i = 0; i < levelSet->count; i++) {
        if (strcmp(levelSet->idNames[i], levelName) == 0) {
            levelSet->visitedList[i] = visited;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x00416f40
int front_level_set_is_level_visited(Front_LevelSet* levelSet, const char* levelName)
{
    int i;

    for (i = 0; i < levelSet->count; i++) {
        if (strcmp(levelSet->idNames[i], levelName) == 0) {
            return levelSet->visitedList[i];
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00416fb0
void front_level_set_set_level_link(Front_LevelSet* levelSet, const char* levelName, Front_LevelLink* link)
{
    int i;

    for (i = 0; i < levelSet->count; i++) {
        if (strcmp(levelSet->idNames[i], levelName) == 0) {
            levelSet->levels[i] = link;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x00417020
Front_LevelLink* front_level_set_get_level_link(Front_LevelSet* levelSet, const char* levelName)
{
    int i;

    for (i = 0; i < levelSet->count; i++) {
        if (strcmp(levelSet->idNames[i], levelName) == 0) {
            return levelSet->levels[i];
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00417090
Front_LevelLink* front_level_set_load_level_links(Front_LevelSet* levelSet, const char* levelName)
{
    Front_LevelLink* link;
    int index;
    char* linksString;
    unsigned int count;
    unsigned int i;
    char* stringParts[15];

    if (levelName != NULL) {
        if (!front_level_set_is_level_visited(levelSet, levelName)) {
            link = (Front_LevelLink*)malloc(sizeof(Front_LevelLink));
            front_level_set_set_level_visited(levelSet, levelName, 1);
            front_level_set_set_level_link(levelSet, levelName, link);
            if (link != NULL) {
                index = front_level_set_index_of(levelSet, levelName);
                if (index != -1) {
                    linksString = block_get_str_copy(legoGlobs_config,
                                                     string_path_join(s_engine_config_root, levelName,
                                                                      // STRING: LEGORR 0x004a26e8
                                                                      "LevelLinks", NULL));
                    link->setIndex = index;
                    i = 0;
                    if (linksString != NULL) {
                        count = sound_variant_split(linksString, stringParts, ",");
                        if (count == 0) {
                            goto failed;
                        }
                        link->linkLevels = (Front_LevelLink**)malloc(count * sizeof(Front_LevelLink*));
                        for (i = 0; i < count; i++) {
                            link->linkLevels[i] = front_level_set_load_level_links(levelSet, stringParts[i]);
                        }
                    } else {
                        link->linkLevels = NULL;
                    }
                    link->linkCount = i;
                    return link;
                }
            failed:
                free(link);
                return NULL;
            }
        } else {
            return front_level_set_get_level_link(levelSet, levelName);
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004171c0
int front_level_link_run_through_links(Front_LevelLink* link, Front_LevelLinkCallback callback, void* data)
{
    unsigned int i;

    if (link != NULL && !link->visited) {
        link->visited = 1;
        if (callback(link, data)) {
            return 1;
        }
        if (link->linkLevels != NULL) {
            for (i = 0; i < link->linkCount; i++) {
                if (front_level_link_run_through_links(link->linkLevels[i], callback, data)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00417240
int front_level_link_callback_inc_count(Front_LevelLink* link, void* data)
{
    (*(unsigned int*)data)++;
    return 0;
}

// FUNCTION: LEGORR 0x00417250
int front_level_link_callback_find_by_link_index(Front_LevelLink* link, void* data)
{
    Front_LevelLinkSearch* search;

    search = (Front_LevelLinkSearch*)data;
    if (search->linkIndex == search->currentIndex) {
        search->resultIndex = search->currentIndex;
        search->resultLink = link;
        return 1;
    }
    search->currentIndex++;
    return 0;
}

// FUNCTION: LEGORR 0x00417280
Front_LevelLink* front_level_link_find_by_link_index(Front_LevelLink* startLink, unsigned int linkIndex)
{
    Front_LevelLinkSearch search;

    search.resultLink = NULL;
    search.linkIndex = linkIndex;
    search.currentIndex = 0;
    search.resultIndex = 0;
    if (front_level_link_run_through_links(startLink, front_level_link_callback_find_by_link_index, &search)) {
        front_levels_reset_visited();
        return search.resultLink;
    }
    front_levels_reset_visited();
    return NULL;
}

// FUNCTION: LEGORR 0x004172d0
int front_level_link_get_set_index(Front_LevelLink* startLink, unsigned int linkIndex)
{
    Front_LevelLink* link;

    link = front_level_link_find_by_link_index(startLink, linkIndex);
    if (link != NULL) {
        return link->setIndex;
    }
    return -1;
}

// FUNCTION: LEGORR 0x004172f0
void front_levels_update_available_recurse(Front_LevelLink* link, Front_LevelUpdate* update,
                                           int previousUnlocked)
{
    int unlocked;
    unsigned int i;

    if (link != NULL && !link->visited) {
        if (FRONT_PROGRAMMER_LEVEL >= 2 || (s_engine_feature_word & 0x20000)) {
            update->setLocked = 0;
        }
        link->visited = 1;
        if (update->rewards != NULL && FRONT_PROGRAMMER_LEVEL < 2 && !(s_engine_feature_word & 0x20000)) {
            unlocked = update->rewards[link->setIndex].flags & 1;
        } else {
            unlocked = (update->setLocked == 0);
        }
        if (!unlocked && !previousUnlocked &&
            !update->select->selItemList[update->index].frontEndOpen) {
            update->select->selItemList[update->index].flags &= ~4;
        } else {
            update->select->selItemList[update->index].flags |= 4;
        }
        update->index++;
        for (i = 0; i < link->linkCount; i++) {
            front_levels_update_available_recurse(link->linkLevels[i], update, unlocked);
        }
    }
}

// FUNCTION: LEGORR 0x004173e0
void front_levels_update_available(Front_LevelLink* startLink, Front_SaveReward* rewards,
                                   Front_LevelSet* levelSet, Front_SelectData* select, int setLocked)
{
    Front_LevelUpdate update;

    update.rewards = rewards;
    update.levelSet = levelSet;
    update.select = select;
    update.index = 0;
    update.setLocked = setLocked;
    front_levels_update_available_recurse(startLink, &update, 1);
    front_levels_reset_visited();
}

// FUNCTION: LEGORR 0x00417430
int front_save_get_level_score(unsigned int index, Front_SaveData* saveData)
{
    return (int)saveData->missionsTable[index].reward.score;
}

// FUNCTION: LEGORR 0x00417460
void front_callback_select_mission_item(float elapsed, int selectIndex)
{
    Front_Font* font = frontGlobs_mainMenuSet->menus[1]->menuFont;
    Front_LevelTextWindow* wnd = frontGlobs_saveLevelWnd;
    Front_LevelLink* link = front_level_link_find_by_link_index(g_front_save_tree, selectIndex);
    const char* levelName = frontGlobs_missionLevels.langNames[link->setIndex];
    char levelNumber[64] = { 0 };
    Front_SaveData* saveData = (Front_SaveData*)front_save_get_current_save_data();
    char buffer[300];

    frontGlobs_prevLevelNumber = frontGlobs_currLevelNumber;
    frontGlobs_currLevelNumber = link->setIndex + 1;
    if (s_engine_feature_word & 0x10000) {
        // STRING: LEGORR 0x004a2728
        sprintf(levelNumber, " (Level %d)", link->setIndex + 1);
    }
    if (levelName == NULL || *levelName == '\0') {
        levelName = frontGlobs_missionLevels.idNames[link->setIndex];
    }
    if (wnd->textWindow != NULL) {
        if (s_loadSaveTextShown) {
            text_buffer_append_format(wnd->textWindow, levelName);
            if (s_engine_feature_word & 0x10000) {
                text_buffer_append_format(wnd->textWindow, levelNumber);
            }
        }
    } else {
        front_util_print_f(font, s_display_w / 2 - (int)formatted_text_printf_width(font, levelName) / 2,
                           390, levelName);
    }

    if (saveData != NULL && (saveData->missionsTable[link->setIndex].flags & 1) &&
        !(saveData->missionsTable[link->setIndex].flags & 2)) {
        // STRING: LEGORR 0x004a270c
        sprintf(buffer, " %s (%i)",
                block_get_strptr(legoGlobs_config, string_path_join(s_engine_config_root, "Menu",
                                                                                                  // STRING: LEGORR 0x004a2718
                                                                                                  "Level_Completed", NULL)),
                front_save_get_level_score(link->setIndex, saveData));
        mission_replace_char(buffer, '_', ' ');
    } else {
        // STRING: LEGORR 0x004a26f4
        sprintf(buffer, " %s",
                block_get_strptr(legoGlobs_config, string_path_join(s_engine_config_root, "Menu",
                                                                                                  // STRING: LEGORR 0x004a26f8
                                                                                                  "Level_Incomplete", NULL)));
        mission_replace_char(buffer, '_', ' ');
    }

    if (wnd->textWindow != NULL && s_loadSaveTextShown) {
        text_buffer_append_format(wnd->textWindow, buffer);
        if (frontGlobs_currLevelNumber != frontGlobs_prevLevelNumber) {
            frontGlobs_playLevelStream = 1;
            frontGlobs_levelStreamTimer = 0.0f;
        }
        if (frontGlobs_playLevelStream && (frontGlobs_levelStreamTimer += elapsed) * 25.0f > 500.0f) {
            if (frontGlobs_currLevelNumber > 8) {
                if (front_play_level_stream(frontGlobs_currLevelNumber - 8)) {
                    frontGlobs_playLevelStream = 0;
                }
            } else if (front_play_tutorial_stream(frontGlobs_currLevelNumber)) {
                frontGlobs_playLevelStream = 0;
            }
        }
    }
    s_loadSaveTextShown = 0;
}

// FUNCTION: LEGORR 0x00417700
void front_callback_select_tutorial_item(float elapsed, int selectIndex)
{
    Front_LevelTextWindow* wnd;
    Front_LevelLink* link;
    const char* levelName;

    wnd = frontGlobs_saveLevelWnd;
    link = front_level_link_find_by_link_index(frontGlobs_startTutorialLink, selectIndex);
    levelName = frontGlobs_tutorialLevels.langNames[link->setIndex];
    frontGlobs_prevLevelNumber = frontGlobs_currLevelNumber;
    frontGlobs_currLevelNumber = link->setIndex + 1;
    if (levelName == NULL || *levelName == '\0') {
        levelName = frontGlobs_tutorialLevels.idNames[link->setIndex];
    }
    if (wnd->textWindow != NULL && s_loadSaveTextShown) {
        text_buffer_append_format(wnd->textWindow, levelName);
        if (frontGlobs_currLevelNumber != frontGlobs_prevLevelNumber) {
            frontGlobs_playLevelStream = 1;
            frontGlobs_levelStreamTimer = 0.0f;
        }
        if (frontGlobs_playLevelStream && (frontGlobs_levelStreamTimer += elapsed) * 25.0f > 500.0f) {
            if (front_play_tutorial_stream(frontGlobs_currLevelNumber)) {
                frontGlobs_playLevelStream = 0;
            }
        }
    }
    s_loadSaveTextShown = 0;
}

// FUNCTION: LEGORR 0x004177e0
int front_level_link_callback_load_menu_item(Front_LevelLink* link, void* data)
{
    const char* levelName;
    int x;
    int y;
    int frontEndOpen;
    char* menuBMP;
    char buffer[1024];

    levelName = ((Front_LevelMenuBuild*)data)->levelSet->idNames[link->setIndex];
    // STRING: LEGORR 0x004a2758
    x = FRONT_CONFIG_INT(string_path_join(s_engine_config_root, levelName, "FrontEndX", NULL));
    // STRING: LEGORR 0x004a274c
    y = FRONT_CONFIG_INT(string_path_join(s_engine_config_root, levelName, "FrontEndY", NULL));
    frontEndOpen = block_get_bool_path(legoGlobs_config,
                                       // STRING: LEGORR 0x004a273c
                                       string_path_join(s_engine_config_root, levelName, "FrontEndOpen", NULL));
    if (frontEndOpen == 2) {
        frontEndOpen = 0;
    }
    // STRING: LEGORR 0x004a2734
    menuBMP = block_get_str_copy(legoGlobs_config, string_path_join(s_engine_config_root, levelName, "MenuBMP", NULL));
    if (menuBMP != NULL) {
        front_menu_item_add_select_item(((Front_LevelMenuBuild*)data)->select, menuBMP, 0, NULL, x, y, frontEndOpen);
        free(menuBMP);
        return 0;
    }
    strcpy(buffer, frontGlobs_defaultLevelBMPS);
    front_menu_item_add_select_item(((Front_LevelMenuBuild*)data)->select, buffer, 0, NULL, x, y, frontEndOpen);
    return 0;
}

// FUNCTION: LEGORR 0x004179b0
void front_levels_load_menu_items(int menuType, Front_LevelLink* startLink, Front_LevelSet* levelSet,
                                  Front_Menu* menu, Front_SaveData* saveData, Front_Menu* nextMenu,
                                  Front_SelectCallback callback)
{
    Front_LevelMenuBuild build;
    unsigned int count;
    Front_SelectData* select;

    build.levelSet = levelSet;
    build.menu = menu;
    build.select = NULL;
    build.saveData = saveData;
    count = 0;
    front_level_link_run_through_links(startLink, front_level_link_callback_inc_count, &count);
    front_levels_reset_visited();
    select = front_menu_item_create_select(&frontGlobs_mainMenuValues[menuType], s_config_angle_empty,
                                           s_config_angle_empty, 0, 0, 0, count, 0, 0, 0, 0, 0,
                                           callback, nextMenu);
    front_menu_add_menu_item(menu,
        // STRING: LEGORR 0x004a2764
        front_menu_item_create_banner_item("Levels!!!", NULL, NULL, 0, 0, Front_MenuItem_Select, 1,
                                           select, 0));
    build.select = select;
    front_level_link_run_through_links(startLink, front_level_link_callback_load_menu_item, &build);
    front_levels_reset_visited();
}

// FUNCTION: LEGORR 0x00417a90
int front_save_read_save_file(uint32_t saveIndex, void* outData, int readOnly)
{
    char buffer[100];
    File* file;
    unsigned int missionsCount;
    unsigned int tutorialsCount;

    if (saveIndex < 6) {
        // STRING: LEGORR 0x004a2774
        sprintf(buffer, "%s\\%i.sav",
                                      // STRING: LEGORR 0x004a2780
                                      "Saves", saveIndex);
        file = file_open(buffer, "rb");
        if (file != NULL) {
            missionsCount = 0;
            tutorialsCount = 0;
            file_read(outData, 0xb0, 1, file);
            front_level_link_run_through_links(g_front_save_tree,
                                               front_level_link_callback_inc_count, &missionsCount);
            front_levels_reset_visited();
            front_level_link_run_through_links(frontGlobs_startTutorialLink,
                                               front_level_link_callback_inc_count, &tutorialsCount);
            front_levels_reset_visited();
            if (((Front_SaveData*)outData)->missionsCount != missionsCount && !readOnly) {
                front_save_write_save_file(saveIndex, NULL);
                memset(outData, 0, sizeof(Front_SaveData));
                return 0;
            }
            ((Front_SaveData*)outData)->missionsTable = (Front_SaveReward*)malloc(((Front_SaveData*)outData)->missionsCount * sizeof(Front_SaveReward));
            file_read(((Front_SaveData*)outData)->missionsTable, sizeof(Front_SaveReward), ((Front_SaveData*)outData)->missionsCount, file);
            file_close(file);
            return 1;
        }
        if (!readOnly) {
            front_save_write_save_file(saveIndex, NULL);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00417bd0
int front_save_write_save_file(unsigned int saveIndex, Front_SaveData* saveData)
{
    char buffer[100];
    File* file;
    unsigned int missionsCount;
    unsigned int i;

    if (saveIndex < 6) {
        sprintf(buffer, "%s\\%i.sav", "Saves", saveIndex);
        if (saveData != NULL) {
            missionsCount = 0;
            front_level_link_run_through_links(g_front_save_tree,
                                               front_level_link_callback_inc_count, &missionsCount);
            front_levels_reset_visited();
            if (missionsCount != saveData->missionsCount) {
                front_save_write_save_file(saveIndex, NULL);
                return 1;
            }
            // STRING: LEGORR 0x004a2794
            file = file_open(buffer, "wb");
            if (file != NULL) {
                file_write(saveData, 0xb0, 1, file);
                if (saveData->missionsCount != 0) {
                    file_write(saveData->missionsTable, sizeof(Front_SaveReward), saveData->missionsCount, file);
                }
                file_close(file);
                // STRING: LEGORR 0x004a2788
                sprintf(buffer, "%s\\%i.osf", "Saves", saveIndex);
                front_object_recall_save_adapter(buffer);
                return 1;
            }
        } else {
            file_make_dir("Saves");
            file = file_open(buffer, "wb");
            if (file != NULL) {
                Front_SaveData newData = { 0 };
                Front_SaveReward newReward = { 0 };

                newData.sfxVolume = 7;
                newData.musicVolume = 5;
                newData.brightness = 5;
                newData.gameSpeed = 1;
                newData.missionsCount = 0;
                front_level_link_run_through_links(g_front_save_tree,
                                                   front_level_link_callback_inc_count, &newData.missionsCount);
                front_levels_reset_visited();
                file_write(&newData, 0xb4, 1, file);
                for (i = 0; i < newData.missionsCount; i++) {
                    file_write(&newReward, sizeof(Front_SaveReward), 1, file);
                }
                file_close(file);
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00417df0
void front_save_read_save_files(void)
{
    unsigned int i;

    for (i = 0; i < 6; i++) {
        if (!(s_engine_feature_word & 0x400)) {
            if (front_save_read_save_file(i, &frontGlobs_saveData[i], 0)) {
                continue;
            }
        } else {
            front_save_write_save_file(i, NULL);
        }
        front_save_read_save_file(i, &frontGlobs_saveData[i], 0);
    }
}

// FUNCTION: LEGORR 0x00417e50
void* front_save_get_save_data_at_index(int saveIndex)
{
    if (saveIndex < 6 && saveIndex >= 0) {
        return &frontGlobs_saveData[saveIndex];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00417e70
void* front_save_get_current_save_data(void)
{
    if (frontGlobs_saveNumber != -1) {
        return front_save_get_save_data_at_index(frontGlobs_saveNumber);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00417e90
int front_save_get_save_number(void)
{
    return frontGlobs_saveNumber;
}

// FUNCTION: LEGORR 0x00417ea0
void front_save_set_save_number(int saveNumber)
{
    frontGlobs_saveNumber = saveNumber;
}

// FUNCTION: LEGORR 0x00417eb0
void front_save_set_level_completed(unsigned int levelIndex)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_current_save_data();
    if (saveData != NULL) {
        if (!front_is_tutorial_selected()) {
            if (levelIndex >= 8) {
                if (levelIndex <= saveData->missionsCount) {
                    saveData->missionsTable[levelIndex].flags |= 1;
                }
                return;
            }
        } else if (levelIndex >= 8) {
            return;
        }
        saveData->missionsTable[levelIndex].flags |= 1;
        saveData->missionsTable[levelIndex].flags |= 2;
    }
}

// FUNCTION: LEGORR 0x00417f20
void front_save_set_save_struct_18(const Front_SaveStruct18* saveStruct18)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_current_save_data();
    if (saveData != NULL) {
        saveData->saveStruct18 = *saveStruct18;
    }
}

// FUNCTION: LEGORR 0x00417f40
int front_save_set_reward_level(int levelIndex, const uint8_t* rewardLevel)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_current_save_data();
    if (saveData != NULL && levelIndex < (int)saveData->missionsCount) {
        saveData->missionsTable[levelIndex].reward = *(const Front_RewardLevel*)rewardLevel;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00417f90
void* front_save_get_reward_level(int levelIndex)
{
    Front_SaveData* saveData;

    saveData = (Front_SaveData*)front_save_get_current_save_data();
    if (saveData != NULL && levelIndex < (int)saveData->missionsCount) {
        return &saveData->missionsTable[levelIndex].reward;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00417fc0
void front_save_write_current_save_file(void)
{
    front_save_write_save_file(frontGlobs_saveNumber, FRONT_CURRENT_SAVE());
}

// FUNCTION: LEGORR 0x00417fe0
int front_is_load_save_menu(void)
{
    return frontGlobs_isLoadSaveMenu;
}

// FUNCTION: LEGORR 0x00417ff0
void front_set_load_save_menu(int isLoadSaveMenu)
{
    frontGlobs_isLoadSaveMenu = isLoadSaveMenu;
}

// FUNCTION: LEGORR 0x00418000
void front_save_reset_current_save(void)
{
    front_save_write_save_file(front_save_get_save_number(), NULL);
    front_levels_update_available(g_front_save_tree, NULL, &frontGlobs_missionLevels,
        (Front_SelectData*)frontGlobs_mainMenuSet->menus[1]->items[1]->itemData, 1);
}

// FUNCTION: LEGORR 0x00418040
void front_save_copy_save_data(Front_SaveData* outData)
{
    if (front_save_get_current_save_data() != NULL) {
        *outData = *FRONT_CURRENT_SAVE();
        outData->missionsTable = (Front_SaveReward*)malloc(outData->missionsCount * sizeof(Front_SaveReward));
        if (outData->missionsTable != NULL) {
            memcpy(outData->missionsTable, FRONT_CURRENT_SAVE()->missionsTable,
                   outData->missionsCount * sizeof(Front_SaveReward));
        }
    }
}

// FUNCTION: LEGORR 0x004180c0
void front_save_restore_save_data(Front_SaveData* data)
{
    if (front_save_get_current_save_data() != NULL) {
        if (FRONT_CURRENT_SAVE()->missionsTable != NULL) {
            free(FRONT_CURRENT_SAVE()->missionsTable);
        }
        *FRONT_CURRENT_SAVE() = *data;
        FRONT_CURRENT_SAVE()->missionsTable = data->missionsTable;
    }
}

// FUNCTION: LEGORR 0x00418110
void front_levels_set_update(uint32_t update)
{
    frontGlobs_levelSelectUpdate = update;
}

// FUNCTION: LEGORR 0x00418120
int front_save_is_game_100_percented(void)
{
    unsigned int total;
    unsigned int i;

    if (front_save_get_current_save_data() != NULL) {
        total = 0;
        for (i = 8; i < FRONT_CURRENT_SAVE()->missionsCount; i++) {
            total += (int)FRONT_CURRENT_SAVE()->missionsTable[i].reward.score;
        }
        if (total / (FRONT_CURRENT_SAVE()->missionsCount - 8) > 99) {
            return 1;
        }
    }
    return 0;
}
