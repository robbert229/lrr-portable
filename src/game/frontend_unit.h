// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-017 FrontEnd.c, rebuilt in original shape (ADR 0005; finding 623).
 *
 * C boundary header: the original's front-end records and the unit's
 * functions, with C linkage. Part 1 (004102b0-00414de0) is in
 * frontend_unit.c; part 2 (00414df0-00418190) is still the relocated port
 * code in frontend_port.cpp. Only C units, frontend_unit.c and
 * src/game/test_c_boundaries.cpp include this header, so the other units'
 * symbol numbering does not change (finding 538).
 *
 * The record names follow OpenLRR's FrontEnd.h with a Front_ prefix, which
 * keeps them apart from the port's C++ types in the boundary test. */
#ifndef LEGORRR_GAME_FRONTEND_UNIT_H
#define LEGORRR_GAME_FRONTEND_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Image;
struct TextureSurface;

/* The fonts are gods98 Font.c records; the port's font entries take its
 * TextureSurface, so the unit passes them as that. */
typedef struct TextureSurface Front_Font;

/* Each menu item kind's data (MenuItem +0x10, by MenuItem +0x14). */
typedef enum Front_MenuItemType {
    Front_MenuItem_Cycle = 0,
    Front_MenuItem_Trigger = 1,
    Front_MenuItem_TextInput = 2,
    Front_MenuItem_Slider = 3,
    Front_MenuItem_RealSlider = 4,
    Front_MenuItem_Next = 5,
    Front_MenuItem_Select = 6
} Front_MenuItemType;

typedef void (*Front_IntCallback)(int value);
typedef void (*Front_RealCallback)(float value);
typedef void (*Front_TriggerCallback)(void);
typedef void (*Front_SelectCallback)(float elapsed, int index);

/* 0x1c bytes (00410a10). */
typedef struct Front_CycleData {
    char** nameList;            /* +0x00 */
    int nameCapacity;           /* +0x04 */
    int nameCount;              /* +0x08 */
    int* valuePtr;              /* +0x0c */
    int x2;                     /* +0x10 */
    int y2;                     /* +0x14 */
    Front_IntCallback callback; /* +0x18 */
} Front_CycleData;

/* 0xc bytes (00410b60). */
typedef struct Front_TriggerData {
    int* valuePtr;                  /* +0x00 */
    int end;                        /* +0x04 */
    Front_TriggerCallback callback; /* +0x08 */
} Front_TriggerData;

/* MenuItem type 2 (part 2 makes it). */
typedef struct Front_TextInputData {
    char* text;                 /* +0x00 */
    int length;                 /* +0x04 */
    int caretPos;               /* +0x08 */
    int bufferLength;           /* +0x0c */
    int x2;                     /* +0x10 */
    int y2;                     /* +0x14 */
} Front_TextInputData;

/* 0x38 bytes (00410bf0). */
typedef struct Front_SliderData {
    int* valuePtr;                  /* +0x00 */
    int valueMin;                   /* +0x04 */
    int valueMax;                   /* +0x08 */
    int x2;                         /* +0x0c */
    int y2;                         /* +0x10 */
    Front_IntCallback callback;     /* +0x14 */
    struct Image* imageBarOff;      /* +0x18 */
    struct Image* imageBarOn;       /* +0x1c */
    struct Image* imageCapLeft;     /* +0x20 */
    struct Image* imageCapRight;    /* +0x24 */
    struct Image* imagePlusLo;      /* +0x28 */
    struct Image* imageMinusLo;     /* +0x2c */
    struct Image* imagePlusHi;      /* +0x30 */
    struct Image* imageMinusHi;     /* +0x34 */
} Front_SliderData;

/* 0x1c bytes (00410b90). */
typedef struct Front_RealSliderData {
    float* valuePtr;                /* +0x00 */
    float valueMin;                 /* +0x04 */
    float valueMax;                 /* +0x08 */
    float step;                     /* +0x0c */
    int x2;                         /* +0x10 */
    int y2;                         /* +0x14 */
    Front_RealCallback callback;    /* +0x18 */
} Front_RealSliderData;

/* One entry of a select list, 0x20 bytes (00410740). Flag 1: a text
 * entry (banner); flag 2: an image entry (images); flag 4: selectable. */
typedef struct Front_SelectItem {
    unsigned int flags;         /* +0x00 */
    char* banner;               /* +0x04 */
    struct Image* images[3];    /* +0x08 */
    int xPos;                   /* +0x14 */
    int yPos;                   /* +0x18 */
    int frontEndOpen;           /* +0x1c */
} Front_SelectItem;

/* 0x5c bytes (004105f0). */
typedef struct Front_SelectData {
    Front_SelectItem* selItemList;  /* +0x00 */
    int* widths[3];                 /* +0x04 */
    int* heights[3];                /* +0x10 */
    int selItemCount;               /* +0x1c */
    char* string1;                  /* +0x20 the scroll-up label */
    char* string2;                  /* +0x24 the scroll-down label */
    int* valuePtr;                  /* +0x28 */
    int x1;                         /* +0x2c */
    int y1;                         /* +0x30 */
    int yStep;                      /* +0x34 */
    int visibleCount;               /* +0x38 */
    int xString1;                   /* +0x3c */
    int yString1;                   /* +0x40 */
    int xString2;                   /* +0x44 */
    int yString2;                   /* +0x48 */
    int scrollStart;                /* +0x4c */
    int field_50;                   /* +0x50 */
    Front_SelectCallback callback;  /* +0x54 */
    struct Front_Menu* nextMenu;    /* +0x58 */
} Front_SelectData;

/* 0x3c bytes (00410d50, 00410e20). */
typedef struct Front_MenuItem {
    char* banner;               /* +0x00 */
    int length;                 /* +0x04 */
    Front_Font* fontHi;         /* +0x08 */
    Front_Font* fontLo;         /* +0x0c */
    void* itemData;             /* +0x10 */
    int itemType;               /* +0x14 Front_MenuItemType */
    int x1;                     /* +0x18 */
    int y1;                     /* +0x1c */
    int centerOffLo;            /* +0x20 */
    int centerOffHi;            /* +0x24 */
    int isImageItem;            /* +0x28 */
    struct Image* imageLo;      /* +0x2c */
    struct Image* imageHi;      /* +0x30 */
    int toolTipType;            /* +0x34 */
    int notInTuto;              /* +0x38 */
} Front_MenuItem;

/* 0x20 bytes (00413f90): an animation played over a menu. */
typedef struct Front_MenuOverlay {
    char* filename;                     /* +0x00 */
    int overlayType;                    /* +0x04 1 avi, 2 bmp, 3 lws */
    int field_8;                        /* +0x08 */
    int x;                              /* +0x0c */
    int y;                              /* +0x10 */
    int sfxType;                        /* +0x14 */
    struct Front_MenuOverlay* next;     /* +0x18 */
    int field_1c;                       /* +0x1c */
} Front_MenuOverlay;

/* 0xa0 bytes (00411100). */
typedef struct Front_Menu {
    char* title;                    /* +0x00 */
    char* fullName;                 /* +0x04 */
    int titleLength;                /* +0x08 */
    Front_Font* menuFont;           /* +0x0c */
    struct Image* menuImage;        /* +0x10 */
    struct Image* menuImageDark;    /* +0x14 */
    Front_MenuItem** items;         /* +0x18 */
    int itemCount;                  /* +0x1c */
    int itemCapacity;               /* +0x20 */
    int itemFocus;                  /* +0x24 */
    int closed;                     /* +0x28 */
    int xPos;                       /* +0x2c */
    int yPos;                       /* +0x30 */
    unsigned int flags;             /* +0x34 1 image position set, 2 can scroll */
    float imagePos[2];              /* +0x38 */
    Front_MenuOverlay* overlays;    /* +0x40 */
    int autoCenter;                 /* +0x44 */
    int displayTitle;               /* +0x48 */
    int anchored;                   /* +0x4c */
    int anchoredX;                  /* +0x50 */
    int anchoredY;                  /* +0x54 */
    int centerOffset;               /* +0x58 */
    char name[64];                  /* +0x5c */
    int playRandom;                 /* +0x9c */
} Front_Menu;

/* 8 bytes (00414070). */
typedef struct Front_MenuSet {
    Front_Menu** menus;             /* +0x00 */
    int menuCount;                  /* +0x04 */
} Front_MenuSet;

/* 0x10 bytes (00410490): the image and font cache, by file name. */
typedef struct Front_Cache {
    char* path;                     /* +0x00 */
    struct Image* image;            /* +0x04 */
    Front_Font* font;               /* +0x08 */
    struct Front_Cache* next;       /* +0x0c */
} Front_Cache;

/* Part 1 (004102b0-00414de0). */
char* front_util_replace_text_spaces(const char* str);              /* 004102b0 */
void front_util_print_f(Front_Font* font, int x, int y,
                        const char* msg, ...);                      /* 00410320 */
void front_callback_trigger_play_credits(void);                     /* 004103d0 */
void front_rock_wipe_play(void);                                    /* 004103f0 */
void front_rock_wipe_stop(void);                                    /* 00410440 */
Front_Cache* front_cache_find_by_name(const char* name);            /* 00410450 */
Front_Cache* front_cache_create(const char* name);                  /* 00410490 */
struct Image* front_cache_load_image(const char* name);             /* 00410520 */
Front_Font* front_cache_load_font(const char* name);                /* 00410560 */
char* front_util_string_copy(const char* str);                      /* 004105a0 */
Front_SelectData* front_menu_item_create_select(
    int* valuePtr, const char* string1, const char* string2, int x1,
    int y1, int yStep, int visibleCount, int xString1, int yString1,
    int xString2, int yString2, int field_50,
    Front_SelectCallback callback, Front_Menu* nextMenu);           /* 004105f0 */
void front_menu_item_free_select(Front_SelectData* select);         /* 00410690 */
void front_menu_item_add_select_item(Front_SelectData* select,
                                     char* banner, int frontEndOpen,
                                     Front_Font* font, int xPos,
                                     int yPos, int field_1c);       /* 00410740 */
Front_CycleData* front_menu_item_create_cycle(
    int count, int* valuePtr, int x2, int y2,
    Front_IntCallback callback);                                    /* 00410a10 */
void front_menu_item_add_cycle_item(Front_CycleData* cycle,
                                    const char* name);              /* 00410aa0 */
void front_menu_item_free_cycle(Front_CycleData* cycle);            /* 00410ae0 */
unsigned int renderer_word_a_get(void);                             /* 00410b30 */
unsigned int renderer_word_b_get(void);                             /* 00410b40 */
unsigned int renderer_word_c_get(void);                             /* 00410b50 */
Front_TriggerData* front_menu_item_create_trigger(
    int* valuePtr, int end, Front_TriggerCallback callback);        /* 00410b60 */
Front_RealSliderData* front_menu_item_create_real_slider(
    float* valuePtr, float valueMin, float valueMax, float step,
    int x2, int y2, Front_RealCallback callback);                   /* 00410b90 */
Front_SliderData* front_menu_item_create_slider(
    int* valuePtr, int valueMin, int valueMax, int x2, int y2,
    Front_IntCallback callback, struct Image* barOff,
    struct Image* barOn, struct Image* capLeft, struct Image* capRight,
    struct Image* plusLo, struct Image* minusLo, struct Image* plusHi,
    struct Image* minusHi);                                         /* 00410bf0 */
void front_menu_item_free_menu_item(Front_MenuItem* item);          /* 00410c70 */
Front_MenuItem* front_menu_item_create_banner_item(
    const char* banner, Front_Font* fontLo, Front_Font* fontHi,
    int x1, int y1, int itemType, int centered, void* itemData,
    int notInTuto);                                                 /* 00410d50 */
Front_MenuItem* front_menu_item_create_image_item(
    const char* banner, Front_Font* fontLo, Front_Font* fontHi,
    const char* loImageName, const char* hiImageName, int x1, int y1,
    int itemType, int centered, const char* toolTipName,
    void* itemData);                                                 /* 00410e20 */
void front_menu_free_menu(Front_Menu* menu);                        /* 00410f30 */
int front_menu_load_menu_image(Front_Menu* menu, const char* filename,
                               int light);                          /* 00410fb0 */
Front_Menu* front_menu_create_menu(const char* title,
                                   const char* fullName,
                                   Front_Font* menuFont, int xPos,
                                   int yPos, int autoCenter,
                                   int displayTitle, int centerOffset,
                                   int canScroll,
                                   const char* anchored);           /* 00411100 */
int front_menu_add_menu_item(Front_Menu* menu, Front_MenuItem* item); /* 00411260 */
int front_maths_is_point_inside_rect(int ptX, int ptY, int rcX, int rcY,
                                     int rcWidth, int rcHeight);    /* 004112e0 */
int front_maths_is_point_inside_rect_centered(int ptX, int ptY, int rcX,
                                              int rcY, int rcWidth,
                                              int rcHeight,
                                              int centered);        /* 00411320 */
int front_menu_item_select_test_string_collision(
    Front_Menu* menu, Front_MenuItem* item,
    Front_SelectData* select);                                      /* 00411360 */
int front_menu_is_level_item_under_mouse(Front_Menu* menu, int index); /* 004114f0 */
int front_menu_get_item_bounds(Front_Menu* menu, int index, int* rcX,
                               int* rcY, int* rcWidth,
                               int* rcHeight);                      /* 00411530 */
int front_menu_find_item_under_mouse(Front_Menu* menu, int* index); /* 00411790 */
int front_menu_was_mouse_released(void);                            /* 00411840 */
int front_menu_item_slider_handle_input(Front_Menu* menu,
                                        Front_MenuItem* item,
                                        Front_SliderData* slider);  /* 00411870 */
int front_menu_item_check_not_in_tuto_any_tutorial_flags(
    Front_MenuItem* item);                                          /* 004119d0 */
Front_Menu* front_menu_update_menu_items_input(float elapsed,
                                               Front_Menu* menu);   /* 00411a00 */
unsigned int input_key_context_check(int key);                      /* 00411f00 */
void front_menu_item_draw_slider(Front_SliderData* slider, unsigned int x,
                                 unsigned int y, unsigned int value,
                                 unsigned int range, float brightness);             /* 00411f10 */
int front_menu_get_overlay_count(Front_Menu* menu);                 /* 00412170 */
int front_menu_should_random_play(void);                            /* 00412190 */
void front_menu_update_overlays(Front_Menu* menu);                  /* 004121b0 */
void front_menu_item_draw_select_item(int x, int y, Front_Font* font,
                                      Front_SelectData* select,
                                      int selIndex, int imageIndex); /* 00412450 */
void front_menu_item_draw_save_image(Front_Menu* menu, int selIndex,
                                     Front_SelectData* select,
                                     int bigSize);                  /* 004124f0 */
void front_menu_item_draw_select_text_window(Front_Menu** menu,
                                             Front_Menu** nextMenu,
                                             Front_Menu** origMenu); /* 00412750 */
void front_menu_draw_load_save_text(Front_Menu** menu);             /* 004129d0 */
void front_menu_draw_menu_image(Front_Menu* menu, int light);       /* 00412af0 */
Front_Menu* front_menu_update(float elapsed, Front_Menu* menu,
                              int* menuTransition);                 /* 00412c00 */
void front_menu_update_mouse_position(Front_Menu* menu, float elapsed);            /* 00413970 */
void front_menu_load_slot_images(void);                             /* 00413a60 */
void front_menu_free_slot_images(void);                             /* 00413b50 */
void front_run_screen_menu(Front_Menu* menu);                       /* 00413b80 */
void front_screen_menu_loop(Front_MenuSet* menuSet, int menuIndex); /* 00413e20 */
int front_menu_item_parse_type_string(const char* itemTypeName);    /* 00413e60 */
char* mission_replace_char(char* text, char from, char to);         /* 00413f00 */
int front_menu_overlay_get_type(Front_MenuOverlay* overlay);        /* 00413f30 */
Front_MenuOverlay* front_menu_create_overlay(const char* filename,
                                             Front_MenuOverlay** list,
                                             int x, int y,
                                             int sfxType);          /* 00413f90 */
void front_menu_item_load_slider_images(int arrayLength,
                                        char** stringParts,
                                        struct Image** images);     /* 00414010 */
Front_MenuSet* front_menu_set_create(int menuCount);                /* 00414070 */
Front_MenuSet* front_load_menu_set(struct BlockRec* config,
                                   const char* menuName, ...);      /* 004140c0 */
int front_get_menu_id_by_name(Front_MenuSet* menuSet,
                              const char* name);                    /* 00414c90 */
int front_is_intros_enabled(void);                                  /* 00414ce0 */
void front_callback_slider_brightness(int brightness);              /* 00414d30 */
void front_callback_slider_sound_volume(int volume);                /* 00414de0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_UNIT_H */
