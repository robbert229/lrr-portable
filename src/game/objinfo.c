// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-031: the object overlays (the original's ObjInfo.c, the name of its
 * Lego.cfg block), rebuilt in original shape (ADR 0005, finding 633). Text
 * 00459530-0045a030, .rdata 0049e5b8-0049e5d4, strings 004a8290-004a8375,
 * .bss 004ffe68-004ffee0.
 *
 * ObjInfo_Initialise reads Lego.cfg's ObjInfo block: the health bar's
 * offset, size, border and colours (the border's light and dark shades are
 * the border colour plus and minus 40%), the five hunger images and their
 * offset, and the thought bubbles' offset. Bubbles.c then draws the
 * overlays at each object's screen position.
 *
 * The port's models of the same functions stay in objinfo_port.cpp,
 * unannotated: natively they are the live ones and nothing calls the unit.
 * In the matching build four functions take the port's names
 * (objinfo_link_names.h). */
#include "objinfo_link_names.h"

#include <stdio.h>
#include <stdlib.h>

#include "crt/vc5names.h"
#include "objinfo.h"
#include "objinfo_imports.h"

#define OBJINFO_HUNGERIMAGES 5

/* ObjInfo_Globs flags. */
#define OBJINFO_GLOB_FLAG_HUNGERIMAGES 0x1
#define OBJINFO_GLOB_FLAG_HEALTHBAR 0x2
#define OBJINFO_GLOB_FLAG_HEALTHBAR_VERTICAL 0x8
#define OBJINFO_GLOB_FLAG_BUBBLEIMAGES 0x10

/* The border's shades: [channel][normal, light, dark]. */
#define OBJINFO_SHADE_NORMAL 0
#define OBJINFO_SHADE_LIGHT 1
#define OBJINFO_SHADE_DARK 2

/* The most border lines Draw_LineListEx gets: two per border pixel. */
#define OBJINFO_MAXBORDERLINES 10

typedef struct ObjInfo_ColourRGBF {
    float red;
    float green;
    float blue;
} ObjInfo_ColourRGBF;

/* ObjInfo.c's .bss (004ffe68-004ffee0), 0x78 bytes. */
typedef struct ObjInfo_Globs {
    unsigned int flags;                         /* +0x00 */
    Point2F HealthBarPosition;                  /* +0x04 */
    Point2F HealthBarWidthHeight;               /* +0x0c */
    unsigned int HealthBarBorderSize;           /* +0x14 */
    float HealthBarBorderRGB[3][3];             /* +0x18 [channel][shade] */
    ObjInfo_ColourRGBF HealthBarRGB;            /* +0x3c */
    ObjInfo_ColourRGBF HealthBarBackgroundRGB;  /* +0x48 */
    Point2F HungerImagesPosition;               /* +0x54 */
    Image* HungerImages[OBJINFO_HUNGERIMAGES];  /* +0x5c */
    Point2F BubbleImagesPosition;               /* +0x70 */
} ObjInfo_Globs;

// GLOBAL: LEGORR 0x004ffe68
static ObjInfo_Globs objInfoGlobs;

// FUNCTION: LEGORR 0x00459530
void objinfo_initialise(struct BlockRec* config, const char* gameName)
{
    objinfo_load_health_bar(config, gameName);
    objinfo_load_hunger(config, gameName);
    objinfo_load_bubble(config, gameName);
}

// FUNCTION: LEGORR 0x00459560
int objinfo_load_health_bar(struct BlockRec* config, const char* gameName)
{
    char* str;
    char* argv[10];
    unsigned int loop;

    if ((str = block_get_str_copy(config, string_path_join(gameName,
            // STRING: LEGORR 0x004a830c
            "ObjInfo",
            // STRING: LEGORR 0x004a8314
            "HealthBarPosition", NULL)))) {
        if (sound_variant_split(str, argv, ":") == 2) {
            objInfoGlobs.HealthBarPosition.x = (float)atoi(argv[0]);
            objInfoGlobs.HealthBarPosition.y = (float)atoi(argv[1]);
            free(str);

            if ((str = block_get_str_copy(config, string_path_join(gameName, "ObjInfo",
                    // STRING: LEGORR 0x004a82f4
                    "HealthBarWidthHeight", NULL)))) {
                if (sound_variant_split(str, argv, ":") == 2) {
                    objInfoGlobs.HealthBarWidthHeight.x = (float)atoi(argv[0]);
                    objInfoGlobs.HealthBarWidthHeight.y = (float)atoi(argv[1]);
                    free(str);

                    objInfoGlobs.HealthBarBorderSize = atoi(block_get_strptr(config, string_path_join(gameName, "ObjInfo",
                            // STRING: LEGORR 0x004a82e0
                            "HealthBarBorderSize", NULL)) ?
                                                            block_get_strptr(config, string_path_join(gameName, "ObjInfo", "HealthBarBorderSize", NULL)) :
                                                            s_config_angle_empty);

                    if (!config_rgb_vector_read(config, string_path_join(gameName, "ObjInfo",
                            // STRING: LEGORR 0x004a82cc
                            "HealthBarBorderRGB", NULL),
                                                &objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_NORMAL],
                                                &objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_NORMAL],
                                                &objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_NORMAL])) {
                        return 0;
                    }

                    objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_LIGHT] = (objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_NORMAL] * 0.4f) + objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_NORMAL];
                    objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_LIGHT] = (objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_NORMAL] * 0.4f) + objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_NORMAL];
                    objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_LIGHT] = (objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_NORMAL] * 0.4f) + objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_NORMAL];
                    objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_DARK] = objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_NORMAL] - (objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_NORMAL] * 0.4f);
                    objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_DARK] = objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_NORMAL] - (objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_NORMAL] * 0.4f);
                    objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_DARK] = objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_NORMAL] - (objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_NORMAL] * 0.4f);

                    for (loop = 0; loop < 3; loop++) {
                        if (objInfoGlobs.HealthBarBorderRGB[0][loop] > 1.0f) objInfoGlobs.HealthBarBorderRGB[0][loop] = 1.0f;
                        if (objInfoGlobs.HealthBarBorderRGB[1][loop] > 1.0f) objInfoGlobs.HealthBarBorderRGB[1][loop] = 1.0f;
                        if (objInfoGlobs.HealthBarBorderRGB[2][loop] > 1.0f) objInfoGlobs.HealthBarBorderRGB[2][loop] = 1.0f;
                        if (objInfoGlobs.HealthBarBorderRGB[0][loop] < 0.0f) objInfoGlobs.HealthBarBorderRGB[0][loop] = 0.0f;
                        if (objInfoGlobs.HealthBarBorderRGB[1][loop] < 0.0f) objInfoGlobs.HealthBarBorderRGB[1][loop] = 0.0f;
                        if (objInfoGlobs.HealthBarBorderRGB[2][loop] < 0.0f) objInfoGlobs.HealthBarBorderRGB[2][loop] = 0.0f;
                    }

                    if (!config_rgb_vector_read(config, string_path_join(gameName, "ObjInfo",
                            // STRING: LEGORR 0x004a82b4
                            "HealthBarBackgroundRGB", NULL),
                                                &objInfoGlobs.HealthBarBackgroundRGB.red,
                                                &objInfoGlobs.HealthBarBackgroundRGB.green,
                                                &objInfoGlobs.HealthBarBackgroundRGB.blue)) {
                        return 0;
                    }
                    if (!config_rgb_vector_read(config, string_path_join(gameName, "ObjInfo",
                            // STRING: LEGORR 0x004a82a4
                            "HealthBarRGB", NULL),
                                                &objInfoGlobs.HealthBarRGB.red,
                                                &objInfoGlobs.HealthBarRGB.green,
                                                &objInfoGlobs.HealthBarRGB.blue)) {
                        return 0;
                    }

                    if (block_get_bool_path(config, string_path_join(gameName, "ObjInfo",
                            // STRING: LEGORR 0x004a8290
                            "HealthBarVertical", NULL)) == 1) {
                        objInfoGlobs.flags |= OBJINFO_GLOB_FLAG_HEALTHBAR_VERTICAL;
                    }

                    objInfoGlobs.flags |= OBJINFO_GLOB_FLAG_HEALTHBAR;
                    return 1;
                }
                free(str);
            }
            return 0;
        }
        free(str);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00459900
int objinfo_load_hunger(struct BlockRec* config, const char* gameName)
{
    char* str;
    char* argv[10];

    objinfo_load_hunger_images(config, gameName);

    if ((str = block_get_str_copy(config, string_path_join(gameName, "ObjInfo",
            // STRING: LEGORR 0x004a8328
            "HungerImagesPosition", NULL)))) {
        if (sound_variant_split(str, argv, ":") == 2) {
            objInfoGlobs.HungerImagesPosition.x = (float)atoi(argv[0]);
            objInfoGlobs.HungerImagesPosition.y = (float)atoi(argv[1]);
            free(str);
            objInfoGlobs.flags |= OBJINFO_GLOB_FLAG_HUNGERIMAGES;
            return 1;
        }
        free(str);
    }
    return 0;
}

// FUNCTION: LEGORR 0x004599c0
void objinfo_load_hunger_images(struct BlockRec* config, const char* gameName)
{
    unsigned int loop;
    char name[64];

    for (loop = 0; loop < OBJINFO_HUNGERIMAGES; loop++) {
        // STRING: LEGORR 0x004a8350
        sprintf(name, "HungerImage%i", loop);
        objInfoGlobs.HungerImages[loop] = image_load_bmp_scaled(block_get_strptr(config, string_path_join(gameName, "ObjInfo",
                // STRING: LEGORR 0x004a8340
                "HungerImages", name, NULL)), 0, 0);
        if (objInfoGlobs.HungerImages[loop]) {
            image_surface_blit_transformed((struct TextureSurface*)objInfoGlobs.HungerImages[loop], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
}

// FUNCTION: LEGORR 0x00459a50
int objinfo_load_bubble(struct BlockRec* config, const char* gameName)
{
    char* str;
    char* argv[10];

    if ((str = block_get_str_copy(config, string_path_join(gameName, "ObjInfo",
            // STRING: LEGORR 0x004a8360
            "BubbleImagesPosition", NULL)))) {
        if (sound_variant_split(str, argv, ":") == 2) {
            objInfoGlobs.BubbleImagesPosition.x = (float)atoi(argv[0]);
            objInfoGlobs.BubbleImagesPosition.y = (float)atoi(argv[1]);
            free(str);
            objInfoGlobs.flags |= OBJINFO_GLOB_FLAG_BUBBLEIMAGES;
            return 1;
        }
        free(str);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00459b00
void objinfo_draw_health_bar(struct Lego_Object* liveObj, int screenX, int screenY)
{
    Point2F hiTo[OBJINFO_MAXBORDERLINES];
    Point2F loTo[OBJINFO_MAXBORDERLINES];
    Point2F hiFrom[OBJINFO_MAXBORDERLINES];
    Point2F loFrom[OBJINFO_MAXBORDERLINES];
    unsigned int lineCount = 0;
    Area2F rect;
    float width, height;
    float health;
    unsigned int loop;
    float border;
    float* colour;

    if ((objInfoGlobs.flags & OBJINFO_GLOB_FLAG_HEALTHBAR) &&
        damage_font_live_object_check_can_show_damage((struct EntitySlot*)liveObj)) {
        border = (float)(objInfoGlobs.HealthBarBorderSize * 2);
        width = objInfoGlobs.HealthBarWidthHeight.x + border;
        height = objInfoGlobs.HealthBarWidthHeight.y + border;
        rect.x = (float)screenX + objInfoGlobs.HealthBarPosition.x;
        rect.y = (float)screenY + objInfoGlobs.HealthBarPosition.y;

        for (loop = 0; loop < objInfoGlobs.HealthBarBorderSize; loop++) {
            /* Left and top in the light shade. */
            hiFrom[lineCount].x = rect.x + loop;
            hiFrom[lineCount].y = rect.y + loop;
            hiTo[lineCount].x = hiFrom[lineCount].x;
            hiTo[lineCount].y = (rect.y + height - 2.0f) - loop;
            hiFrom[lineCount + 1].x = (rect.x + width - 1.0f) - loop;
            hiFrom[lineCount + 1].y = rect.y + loop;
            hiTo[lineCount + 1].x = (rect.x + loop) - -1.0f;
            hiTo[lineCount + 1].y = rect.y + loop;
            /* Bottom and right in the dark shade. */
            loFrom[lineCount].x = rect.x + loop;
            loFrom[lineCount].y = (rect.y + height - 1.0f) - loop;
            loTo[lineCount].x = (rect.x + width - 2.0f) - loop;
            loTo[lineCount].y = loFrom[lineCount].y;
            loFrom[lineCount + 1].x = (rect.x + width - 1.0f) - loop;
            loFrom[lineCount + 1].y = (rect.y + height - 1.0f) - loop;
            loTo[lineCount + 1].x = loFrom[lineCount + 1].x;
            loTo[lineCount + 1].y = (rect.y + loop) - -1.0f;
            lineCount += 2;
        }

        health = liveObj->health;
        if (health < 0.0f) health = 0.0f;
        else if (health > 100.0f) health = 100.0f;

        if (objInfoGlobs.flags & OBJINFO_GLOB_FLAG_HEALTHBAR_VERTICAL) {
            rect.height = (1.0f - health * 0.01f) * height;
            rect.width = width;
            surface_fill_pixel_spans(&rect, 1, objInfoGlobs.HealthBarBackgroundRGB.red,
                                     objInfoGlobs.HealthBarBackgroundRGB.green,
                                     objInfoGlobs.HealthBarBackgroundRGB.blue, 0);
            rect.y += rect.height;
            rect.height = height - rect.height;
            colour = &objInfoGlobs.HealthBarRGB.red;
        } else {
            rect.width = health * width * 0.01f;
            rect.height = height;
            surface_fill_pixel_spans(&rect, 1, objInfoGlobs.HealthBarRGB.red,
                                     objInfoGlobs.HealthBarRGB.green,
                                     objInfoGlobs.HealthBarRGB.blue, 0);
            rect.x += rect.width;
            rect.width = width - rect.width;
            colour = &objInfoGlobs.HealthBarBackgroundRGB.red;
        }
        surface_fill_pixel_spans(&rect, 1, colour[0], colour[1], colour[2], 0);

        surface_draw_polyline(hiFrom, hiTo, lineCount,
                              objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_LIGHT],
                              objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_LIGHT],
                              objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_LIGHT], 0);
        surface_draw_polyline(loFrom, loTo, lineCount,
                              objInfoGlobs.HealthBarBorderRGB[0][OBJINFO_SHADE_DARK],
                              objInfoGlobs.HealthBarBorderRGB[1][OBJINFO_SHADE_DARK],
                              objInfoGlobs.HealthBarBorderRGB[2][OBJINFO_SHADE_DARK], 0);
    }
}

// FUNCTION: LEGORR 0x00459f50
void objinfo_draw_hunger(struct Lego_Object* liveObj, int screenX, int screenY)
{
    unsigned int level;
    Point2F pos;

    if (objInfoGlobs.flags & OBJINFO_GLOB_FLAG_HUNGERIMAGES) {
        level = (unsigned int)((liveObj->energy / 100.0f) * OBJINFO_HUNGERIMAGES);
        if (level > 4) level = 4;
        if (objInfoGlobs.HungerImages[level]) {
            pos = objInfoGlobs.HungerImagesPosition;
            pos.x += (float)screenX;
            pos.y += (float)screenY;
            image_display_scaled(objInfoGlobs.HungerImages[level], NULL, &pos, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x00459fd0
void objinfo_draw_bubble(struct Image* image, int screenX, int screenY)
{
    Point2F pos;

    if (objInfoGlobs.flags & OBJINFO_GLOB_FLAG_BUBBLEIMAGES) {
        pos = objInfoGlobs.BubbleImagesPosition;
        pos.x += (float)screenX;
        pos.y += (float)screenY;
        image_display_scaled(image, NULL, &pos, NULL);
    }
}
