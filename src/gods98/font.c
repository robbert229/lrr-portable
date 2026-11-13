// SPDX-License-Identifier: LGPL-3.0-or-later
/* Font.c, original translation unit G98-008 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 605): text 00479ee0-0047a640 and the
 * .bss font pool 00506528-005065b0. The unit's one .rdata constant,
 * 0049e948 (1.0f), is the pooled literal of the width decrement.
 *
 * A font is a 0xbf4-byte record: the bitmap, the source rectangle of each
 * of the 10 x 19 glyph cells (its width trimmed to the glyph's ink), the
 * line height and the tab width. Font_VPrintF2 expands "%b" to an inline
 * image token, formats the text and draws it one glyph at a time, measuring
 * the widest line.
 *
 * The port's text renderer stays in the port tail bitmap_font_port.cpp,
 * unannotated. In the matching build font_link_names.h gives this unit's
 * functions the port's names wherever the port's name has the original's
 * arguments, so their callers reach the unit; natively the port tail serves
 * those names and nothing calls the unit. */
#include "font_link_names.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"
#include "font_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define FONT_GRIDWIDTH 10
#define FONT_GRIDHEIGHT 19
#define FONT_TABWIDTH 8
#define FONT_MAXSTRINGLEN 1024
#define FONT_MAXLISTS 32

/* One font (0xbf4 bytes). posSet[x][y] is the source rectangle of glyph
 * 32 + y * 10 + x; its width is trimmed to the glyph's ink. */
typedef struct Font {
    Image* image;                                 /* +0x000 */
    Area2F posSet[FONT_GRIDWIDTH][FONT_GRIDHEIGHT]; /* +0x004 */
    unsigned int fontHeight;                      /* +0xbe4 */
    unsigned int tabWidth;                        /* +0xbe8 */
    unsigned int reserved;                        /* +0xbec: not read in the binary */
    struct Font* nextFree;                        /* +0xbf0 */
} Font;

Font* font_load(const char* fname);
void font_v_get_string_info(Font* font, unsigned int* width,
                            unsigned int* lineCount, const char* msg,
                            va_list args);
void font_print_f(Font* font, int x, int y, const char* msg, ...);
void font_v_print_f(Font* font, int x, int y, const char* msg, va_list args);
unsigned int font_v_print_f2(Font* font, int x, int y, int render,
                             unsigned int* lineCount, const char* msg,
                             va_list args);
unsigned int font_output_char(Font* font, int x, int y, unsigned char c,
                              int render);
unsigned int font_get_char_width(Font* font, unsigned char c);
unsigned int font_get_height(Font* font);
void font_remove(Font* dead);
Font* font_create(Image* image);
void font_add_list(void);

typedef struct FontGlobs {
    Font* listSet[FONT_MAXLISTS]; /* 00506528 */
    Font* freeList;               /* 005065a8 */
    unsigned int listCount;       /* 005065ac */
} FontGlobs;

// GLOBAL: LEGORR 0x00506528
FontGlobs fontGlobs;

// FUNCTION: LEGORR 0x00479ee0
Font* font_load(const char* fname)
{
    Image* image;
    Font* font;
    unsigned int x, y, xPos;
    unsigned int width, height;
    unsigned int pitch, bpp;
    unsigned int pen255, mask;
    unsigned char* buffer;
    unsigned int offset;

    if (image = image_load_bmp_scaled(fname, 0, 0)) {
        if (font = font_create(image)) {
            image_set_pen_zero_trans(image);
            width = (int)image->width / FONT_GRIDWIDTH;
            height = (int)image->height / FONT_GRIDHEIGHT;
            pen255 = image_get_pen255(image);
            mask = image_get_pixel_mask(image);
            if (buffer = (unsigned char*)image_lock_surface(image, &pitch, &bpp)) {
                for (y = 0; y < FONT_GRIDHEIGHT; y++) {
                    for (x = 0; x < FONT_GRIDWIDTH; x++) {
                        font->posSet[x][y].x = (float)(x * width);
                        font->posSet[x][y].y = (float)(y * height);
                        font->posSet[x][y].width = (float)width;
                        font->posSet[x][y].height = (float)height;
                        for (xPos = width - 1; xPos; xPos--) {
                            offset = (unsigned int)((float)(x * width) + xPos) * (bpp / 8) +
                                     (unsigned int)(float)(y * height) * pitch;
                            if ((((((((buffer[offset] << 8) | buffer[offset + 1]) << 8) |
                                    buffer[offset + 2]) << 8) | buffer[offset + 3]) & mask) == pen255)
                                font->posSet[x][y].width--;
                            else
                                break;
                        }
                    }
                }
                font->fontHeight = (unsigned int)font->posSet[0][0].height;
                font->tabWidth = (unsigned int)font->posSet[0][0].width * FONT_TABWIDTH;
                image_unlock_surface(image);
                return font;
            }
            font_remove(font);
        }
        image_surface_release(image);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0047a150
void font_v_get_string_info(Font* font, unsigned int* width, unsigned int* lineCount,
                            const char* msg, va_list args)
{
    unsigned int w = font_v_print_f2(font, 0, 0, 0, lineCount, msg, args);
    if (width) *width = w;
}

// FUNCTION: LEGORR 0x0047a180
void font_print_f(Font* font, int x, int y, const char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    font_v_print_f2(font, x, y, 1, NULL, msg, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x0047a1b0
void font_v_print_f(Font* font, int x, int y, const char* msg, va_list args)
{
    font_v_print_f2(font, x, y, 1, NULL, msg, args);
}

// FUNCTION: LEGORR 0x0047a1e0
unsigned int font_v_print_f2(Font* font, int x, int y, int render,
                             unsigned int* lineCount, const char* msg,
                             va_list args)
{
    unsigned int lineWidth, lineHeight, loop, maxWidth, lines, count;
    unsigned int value, hex, shift;
    Image* image;
    Point2F pos;
    char line[FONT_MAXSTRINGLEN];
    char msgCopy[FONT_MAXSTRINGLEN];
    char* s;

    lineHeight = font->fontHeight;
    lines = 1;
    lineWidth = 0;
    maxWidth = 0;
    s = msgCopy;
    while (*msg != '\0') {
        if (*msg == '%' && msg[1] == 'b') {
            *s++ = '@';
            *s++ = '[';
            *s++ = '0';
            *s++ = 'x';
            *s++ = *msg;
            *s++ = '0';
            *s++ = '.';
            *s++ = '8';
            *s++ = 'x';
            *s = ']';
            msg++;
        } else {
            *s = *msg;
        }
        msg++;
        s++;
    }
    *s = '\0';

    count = vsprintf(line, msgCopy, args);
    for (loop = 0; loop < count; loop++) {
        if (line[loop] == '\n') {
            if (lineWidth > maxWidth) maxWidth = lineWidth;
            y += lineHeight;
            lineHeight = font->fontHeight;
            lineWidth = 0;
            lines++;
        } else if (line[loop] == '\t') {
            lineWidth += font->tabWidth - (lineWidth % font->tabWidth);
        } else if (loop < count - 12 && line[loop] == '@' && line[loop + 1] == '[' &&
                   line[loop + 2] == '0' && line[loop + 3] == 'x' && line[loop + 12] == ']') {
            value = 0;
            for (hex = 0, shift = 28; hex < 8; hex++) {
                value |= ((unsigned char)line[loop + 4 + hex] -
                          (isdigit((unsigned char)line[loop + 4 + hex]) ? '0' : 'a' - 10)) << shift;
                shift -= 4;
            }
            if (image = (Image*)value) {
                pos.x = (float)(lineWidth + x);
                pos.y = (float)y;
                lineWidth += image->width;
                if (image->height > lineHeight) lineHeight = image->height;
                image_display_scaled(image, NULL, &pos, NULL);
            }
            loop += 12;
        } else {
            lineWidth += font_output_char(font, lineWidth + x, y, line[loop], render);
        }
    }

    if (lineCount) *lineCount = lines;
    return (lineWidth > maxWidth) ? lineWidth : maxWidth;
}

// FUNCTION: LEGORR 0x0047a470
unsigned int font_output_char(Font* font, int x, int y, unsigned char c, int render)
{
    unsigned int xPos, yPos;
    Point2F pos;

    pos.x = (float)x;
    pos.y = (float)y;
    c -= 32;
    yPos = c / FONT_GRIDWIDTH;
    xPos = c % FONT_GRIDWIDTH;
    if (yPos < FONT_GRIDHEIGHT) {
        if (render) image_display_scaled(font->image, &font->posSet[xPos][yPos], &pos, NULL);
        return (unsigned int)font->posSet[xPos][yPos].width;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047a520
unsigned int font_get_char_width(Font* font, unsigned char c)
{
    return font_output_char(font, 0, 0, c, 0);
}

// FUNCTION: LEGORR 0x0047a540
unsigned int font_get_height(Font* font)
{
    return font->fontHeight;
}

// FUNCTION: LEGORR 0x0047a550
void font_remove(Font* dead)
{
    image_surface_release(dead->image);
    dead->nextFree = fontGlobs.freeList;
    fontGlobs.freeList = dead;
}

// FUNCTION: LEGORR 0x0047a580
Font* font_create(Image* image)
{
    Font* newFont;

    if ((newFont = fontGlobs.freeList) == NULL) {
        font_add_list();
        newFont = fontGlobs.freeList;
    }
    fontGlobs.freeList = newFont->nextFree;
    memset(newFont, 0, sizeof(Font));
    newFont->nextFree = newFont;
    newFont->image = image;
    return newFont;
}

// FUNCTION: LEGORR 0x0047a5c0
void font_add_list(void)
{
    Font* list;
    unsigned int count, loop;

    count = 0x00000001 << fontGlobs.listCount;
    if (list = fontGlobs.listSet[fontGlobs.listCount] = (Font*)malloc(sizeof(Font) * count)) {
        fontGlobs.listCount++;
        for (loop = 1; loop < count; loop++) {
            list[loop - 1].nextFree = &list[loop];
        }
        list[count - 1].nextFree = fontGlobs.freeList;
        fontGlobs.freeList = list;
    }
}

#ifdef __cplusplus
}
#endif
