// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-017 TextWindow.c's record, shared with the units that read a
 * window's font or area (Lego.c, Text.c). The +0x... comments are the Win32
 * offsets; the pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GODS98_TEXTWINDOW_RECORD_H
#define LEGORRR_GODS98_TEXTWINDOW_RECORD_H

#include "gods98/font_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEXTWINDOW_MAXSTRINGLEN 1024
#define TEXTWINDOW_MAXLINES 256

typedef struct TextWindowArea {
    float x, y, width, height;
} TextWindowArea;

/* One text window (0x830 bytes on Win32). */
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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_TEXTWINDOW_RECORD_H */
