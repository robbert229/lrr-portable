// SPDX-License-Identifier: LGPL-3.0-or-later
/* TextWindow.c, original translation unit G98-017 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 600): text 004850f0-00485a20 and the
 * .rdata floats 0049ea04 (0.5f) and 0049ea0c (0.0f). The unit has no .data
 * or .bss of its own.
 *
 * A text window is a 0x830-byte record: a font, a screen area, a ring buffer
 * of printed text with the start of each wrapped line, and an overlay string
 * that is shown for a while instead of the ring buffer. TextWindow_Update
 * wraps the ring buffer at word breaks and draws the lines that fit, the
 * newest at the bottom.
 *
 * The port's implementation stays in the port tail text_window_port.cpp,
 * unannotated. In the matching build textwindow_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them; natively
 * the port tail serves those names. */
#include "textwindow_link_names.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"
#include "textwindow_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define TEXTWINDOW_MAXSTRINGLEN 1024
#define TEXTWINDOW_MAXLINES 256

#define TEXTWINDOW_FLAG_WINDOWWRAPPED 0x1
#define TEXTWINDOW_FLAG_OVERLAY 0x2
#define TEXTWINDOW_FLAG_CENTERED 0x4

/* Font.c's record, as far as the unit reads it: Font_GetHeight is an inline
 * read of +0xbe4. */
typedef struct TextWindowFontPos {
    float x, y;
} TextWindowFontPos;

typedef struct Font {
    void* image;                         /* +0x000 */
    TextWindowFontPos posSet[10][19];    /* +0x004 */
    TextWindowFontPos sizeSet[10][19];   /* +0x5f4 */
    unsigned int height;                 /* +0xbe4 */
    unsigned int tabWidth;               /* +0xbe8 */
} Font;

#define Font_GetHeight(f) ((f)->height)

typedef struct TextWindowArea {
    float x, y, width, height;
} TextWindowArea;

/* One text window (0x830 bytes). */
typedef struct TextWindow {
    Font* font;                                 /* +0x000 */
    TextWindowArea windowSize;                  /* +0x004 */
    char* windowBuf;                            /* +0x014 */
    char secondaryBuf[TEXTWINDOW_MAXSTRINGLEN]; /* +0x018 */
    unsigned int windowBufLen;                  /* +0x418 */
    unsigned int usedLen;                       /* +0x41c */
    unsigned int lineList[TEXTWINDOW_MAXLINES]; /* +0x420 */
    unsigned int lineCount;                     /* +0x820 */
    unsigned int maxLines;                      /* +0x824 */
    float displayDelay;                         /* +0x828 */
    unsigned int flags;                         /* +0x82c */
} TextWindow;

TextWindow* text_window_create(Font* font, TextWindowArea* size,
                               unsigned int bufferSize);
void text_window_enable_centering(TextWindow* window, int enable);
void text_window_remove(TextWindow* window);
void text_window_change_position(TextWindow* window, int xPos, int yPos);
void text_window_change_size(TextWindow* window, unsigned int width,
                             unsigned int height);
void text_window_page_printf(TextWindow* window, unsigned int page,
                             const char* msg, ...);
void text_window_printf(TextWindow* window, const char* msg, ...);
void text_window_vprintf(TextWindow* window, unsigned int page,
                         const char* msg, va_list args);
void text_window_printf_overlay(TextWindow* window, int oneFrame,
                                const char* msg, ...);
int text_window_update(TextWindow* window, unsigned int posFromEnd,
                       float time, int* lpLinesUsed);
int text_window_update_overlay(TextWindow* window, float time,
                               int* lpLinesUsed);
void text_window_clear(TextWindow* window);
void text_window_get_info(TextWindow* window, unsigned int* linesUsed,
                          unsigned int* linesInWindow);

// FUNCTION: LEGORR 0x004850f0
TextWindow* text_window_create(Font* font, TextWindowArea* size,
                               unsigned int bufferSize)
{
    TextWindow* window;

    if (window = (TextWindow*)malloc(sizeof(TextWindow))) {
        memset(window, 0, sizeof(TextWindow));
        window->font = font;
        window->windowSize = *size;
        window->windowBuf = (char*)malloc(bufferSize);
        window->windowBufLen = bufferSize;
        text_window_enable_centering(window, 1);
    }
    return window;
}

// FUNCTION: LEGORR 0x00485160
void text_window_enable_centering(TextWindow* window, int enable)
{
    if (enable) window->flags |= TEXTWINDOW_FLAG_CENTERED;
    else window->flags &= ~TEXTWINDOW_FLAG_CENTERED;
}

// FUNCTION: LEGORR 0x00485190
void text_window_remove(TextWindow* window)
{
    free(window->windowBuf);
    free(window);
}

// FUNCTION: LEGORR 0x004851b0
void text_window_change_position(TextWindow* window, int xPos, int yPos)
{
    window->windowSize.x = (float)xPos;
    window->windowSize.y = (float)yPos;
}

// FUNCTION: LEGORR 0x004851d0
void text_window_change_size(TextWindow* window, unsigned int width,
                             unsigned int height)
{
    window->windowSize.width = (float)width;
    window->windowSize.height = (float)height;
}

// FUNCTION: LEGORR 0x00485210
void text_window_page_printf(TextWindow* window, unsigned int page,
                             const char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    text_window_vprintf(window, page, msg, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x00485230
void text_window_printf(TextWindow* window, const char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    text_window_vprintf(window, 0, msg, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x00485250
void text_window_vprintf(TextWindow* window, unsigned int page,
                         const char* msg, va_list args)
{
    char buffer[TEXTWINDOW_MAXSTRINGLEN];
    unsigned int len, loop, count = 0;

    len = vsprintf(buffer, msg, args);
    for (loop = 0; loop < len; loop++) {
        if (count == page) break;
        if (buffer[loop] == '\a') count++;
    }
    if (loop != len) {
        for (; loop < len; loop++) {
            window->windowBuf[window->usedLen] = buffer[loop];
            if (++window->usedLen == window->windowBufLen) {
                window->usedLen = 0;
                window->flags |= TEXTWINDOW_FLAG_WINDOWWRAPPED;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00485300
void text_window_printf_overlay(TextWindow* window, int oneFrame,
                                const char* msg, ...)
{
    va_list args;
    char buffer[TEXTWINDOW_MAXSTRINGLEN];

    va_start(args, msg);
    vsprintf(buffer, msg, args);
    va_end(args);

    strcpy(window->secondaryBuf, buffer);
    if (oneFrame) window->displayDelay = 0.0f;
    else window->displayDelay = 25.0f;
    window->flags |= TEXTWINDOW_FLAG_OVERLAY;
}

/* The wrapped lines of the ring buffer, newest at the bottom; posFromEnd
 * scrolls back. The overlay string replaces the buffer while it is shown.
 * The original reuses `start` as the pen's x once the lines are found: that
 * is what puts x in start's stack slot. With a separate x, MSVC pairs x with
 * wordWidth and every slot of the function moves (78% against 87%).
 * posFromEnd's zero case stores `sub`, which is 0 there; the original's
 * register-zero compare (`cmp ecx, ebx; jbe`) is not reached from C (finding
 * 600). The line loop wraps at windowBufLen + 1, as the original's does. */
// FUNCTION: LEGORR 0x00485390
int text_window_update(TextWindow* window, unsigned int posFromEnd,
                       float time, int* lpLinesUsed)
{
    unsigned int loop, sub;
    unsigned int start = 0;
    int lineWidth = 0, wordWidth = 0;
    int y, width, closest;
    unsigned char c;
    unsigned int lineWidths[TEXTWINDOW_MAXLINES];

    if (lpLinesUsed) *lpLinesUsed = 0;

    window->maxLines = (unsigned int)window->windowSize.height /
                       Font_GetHeight(window->font);

    if (!(window->flags & TEXTWINDOW_FLAG_OVERLAY)) {
        closest = window->windowBufLen;
        if (window->flags & TEXTWINDOW_FLAG_WINDOWWRAPPED) {
            for (loop = 0; loop < window->lineCount; loop++) {
                int diff = window->lineList[loop] - window->usedLen;
                if (diff > 0 && diff < closest) {
                    start = window->lineList[loop];
                    closest = diff;
                }
            }
        }

        window->lineCount = 0;
        window->lineList[window->lineCount++] = start;

        for (loop = start; loop != window->usedLen;) {
            c = window->windowBuf[loop];
            if (c == '\a') break;
            if (c == '\n') {
                window->lineList[window->lineCount] = loop + 1;
                lineWidths[window->lineCount - 1] = lineWidth;
                window->lineCount++;
                lineWidth = wordWidth = 0;
            }
            else {
                if (isspace(c)) {
                    wordWidth = 0;
                    window->lineList[window->lineCount] = loop + 1;
                    lineWidths[window->lineCount - 1] = lineWidth;
                }
                width = bitmap_font_glyph_measure_width(window->font, c);
                lineWidth += width;
                wordWidth += width;
                if ((float)lineWidth > window->windowSize.width) {
                    window->lineCount++;
                    lineWidth = wordWidth;
                }
            }
            if (++loop >= window->windowBufLen) loop = 0;
        }
        window->lineList[window->lineCount] = loop;
        lineWidths[window->lineCount - 1] = lineWidth;

        sub = (window->lineCount - window->maxLines) + 1;
        if (sub > 0) posFromEnd %= sub;
        else posFromEnd = sub;

        for (loop = 0; loop < window->lineCount; loop++) {
            start = (int)window->windowSize.x;
            if (window->flags & TEXTWINDOW_FLAG_CENTERED) {
                start += (int)((window->windowSize.width -
                                (float)lineWidths[loop]) * 0.5f);
            }
            if (window->lineCount > window->maxLines &&
                loop >= window->lineCount - posFromEnd - window->maxLines &&
                loop < window->lineCount - posFromEnd) {
                y = (int)window->windowSize.y +
                    (posFromEnd + loop + window->maxLines - window->lineCount) *
                    Font_GetHeight(window->font);
            }
            else if (window->lineCount <= window->maxLines) {
                y = (int)window->windowSize.y +
                    loop * Font_GetHeight(window->font);
            }
            else {
                y = 0;
            }
            if (lpLinesUsed && y > *lpLinesUsed) *lpLinesUsed = y;
            if (y) {
                for (sub = window->lineList[loop];
                     sub != window->lineList[loop + 1];) {
                    start += font_output_char(window->font, start, y,
                                              window->windowBuf[sub], 1);
                    if (++sub == window->windowBufLen + 1) sub = 0;
                }
            }
        }

        if (lpLinesUsed) *lpLinesUsed += Font_GetHeight(window->font);

        return 0;
    }

    return text_window_update_overlay(window, time, lpLinesUsed);
}

/* The overlay string, wrapped like the ring buffer. As in TextWindow_Update,
 * the pen's x reuses a dead variable (lineWidth). `c = buf[sub++]` keeps c in
 * memory in the draw loop, as the original does. */
// FUNCTION: LEGORR 0x00485710
int text_window_update_overlay(TextWindow* window, float time,
                               int* lpLinesUsed)
{
    unsigned int loop = 0, sub, lineCount;
    int lineWidth = 0, wordWidth = 0;
    int y, width;
    int result = 0;
    unsigned char c;
    unsigned int lineList[TEXTWINDOW_MAXLINES];
    unsigned int lineWidths[TEXTWINDOW_MAXLINES];

    lineList[0] = 0;
    lineCount = 1;
    for (; c = window->secondaryBuf[loop]; loop++) {
        if (c == '\n') {
            lineWidths[lineCount - 1] = lineWidth;
            lineList[lineCount++] = loop + 1;
            lineWidth = 0;
            wordWidth = 0;
        }
        else {
            if (isspace(c)) {
                lineWidths[lineCount - 1] = lineWidth;
                wordWidth = 0;
                lineList[lineCount] = loop + 1;
            }
            width = bitmap_font_glyph_measure_width(window->font, c);
            lineWidth += width;
            wordWidth += width;
            if ((float)lineWidth > window->windowSize.width) {
                lineCount++;
                lineWidth = wordWidth;
            }
        }
    }
    lineWidths[lineCount - 1] = lineWidth;
    lineList[lineCount] = loop;

    for (loop = 0; loop < lineCount; loop++) {
        lineWidth = (int)window->windowSize.x;
        if (window->flags & TEXTWINDOW_FLAG_CENTERED) {
            lineWidth += (int)((window->windowSize.width -
                                (float)lineWidths[loop]) * 0.5f);
        }
        if (lineCount > window->maxLines &&
            loop >= lineCount - window->maxLines && loop < lineCount) {
            y = (int)window->windowSize.y +
                (loop - lineCount + window->maxLines) *
                Font_GetHeight(window->font);
            result = 1;
        }
        else if (lineCount <= window->maxLines) {
            y = (int)window->windowSize.y +
                loop * Font_GetHeight(window->font);
        }
        else {
            y = 0;
        }
        if (lpLinesUsed && y > *lpLinesUsed) *lpLinesUsed = y;
        if (y) {
            for (sub = lineList[loop]; sub != lineList[loop + 1];) {
                c = window->secondaryBuf[sub++];
                lineWidth += font_output_char(window->font, lineWidth, y,
                                              c, 1);
            }
        }
    }

    if ((window->displayDelay -= time) < 0.0f) {
        window->flags &= ~TEXTWINDOW_FLAG_OVERLAY;
    }

    return result;
}

// FUNCTION: LEGORR 0x004859b0
void text_window_clear(TextWindow* window)
{
    memset(window->windowBuf, 0, window->windowBufLen);
    memset(window->secondaryBuf, 0, TEXTWINDOW_MAXSTRINGLEN);
    window->lineCount = 0;
    window->usedLen = 0;
    window->flags &= ~TEXTWINDOW_FLAG_WINDOWWRAPPED;
}

// FUNCTION: LEGORR 0x00485a00
void text_window_get_info(TextWindow* window, unsigned int* linesUsed,
                          unsigned int* linesInWindow)
{
    *linesUsed = window->lineCount;
    *linesInWindow = window->maxLines;
}

#ifdef __cplusplus
}
#endif
