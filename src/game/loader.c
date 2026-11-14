// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-025: the loading screen and its progress bar (the original's
 * Loader.c, a name inferred from OpenLRR's), rebuilt in original shape
 * (ADR 0005, finding 633). Text 0044db50-0044e0c0, .rdata 0049e4d4-0049e4e0,
 * strings 004a52ac-004a52b8, commons 00555e00-00556090.
 *
 * Loader_Initialise loads the loading and shutdown screens and the bar
 * image, and reads the profile of section sizes ("Game Data=1234", one per
 * line) written by the previous run. Lego.c names the section being loaded
 * (Loader_display_loading_bar); every file the file layer opens then calls
 * Loader_FileLoadCallback with its size, which redraws the screen and the
 * bar when the section's share has grown. Loader_Shutdown writes the sizes
 * just measured back to the profile.
 *
 * The native port has no DirectDraw back buffer: the unit compiles to
 * nothing there, and lego_unit.c calls the port's entries
 * (lego_unit_imports.h). */
#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "loader.h"
#include "loader_imports.h"

#define LOADER_MAXSECTIONS 50

/* Loader_Globs flags. */
#define LOADER_GLOB_FLAG_ENABLED 0x1

/* Direction: the side the bar grows from. */
#define DIRECTION_UP 0
#define DIRECTION_RIGHT 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3

typedef struct LoaderSection {
    char* name;                 /* +0 */
    unsigned int currentSize;   /* +4 bytes loaded this run */
    unsigned int totalSize;     /* +8 bytes the profile says to expect */
} LoaderSection;

/* Loader.c's commons (00555e00-00556090), 0x290 bytes. */
typedef struct Loader_Globs {
    Image* LoadScreen;                              /* +0x000 */
    struct TextureSurface* font;                    /* +0x004 */
    LoaderSection sectionList[LOADER_MAXSECTIONS];  /* +0x008 */
    LoaderSection* currentSection;                  /* +0x260 */
    Image* ShutdownScreen;                          /* +0x264 */
    Image* progressBar;                             /* +0x268 */
    Area2F progressWindow;                          /* +0x26c */
    int progressDirection;                          /* +0x27c */
    char* loadingText;                              /* +0x280 */
    uint32_t loadingWidth;                          /* +0x284 */
    float progressLast;                             /* +0x288 */
    unsigned int flags;                             /* +0x28c */
} Loader_Globs;

// GLOBAL: LEGORR 0x00555e00
Loader_Globs loaderGlobs;

// FUNCTION: LEGORR 0x0044db50
void loader_initialise(const char* loadScreenName, const char* shutdownScreenName,
                       struct TextureSurface* font5HI, const char* profileName,
                       int progressDirection, const char* progressBarName,
                       Area2F* progressWindow, const char* loadingText)
{
    File* file;
    unsigned int loop;
    char* argv[10];
    char line[256];
    char* s;

    memset(&loaderGlobs, 0, sizeof(loaderGlobs));

    loaderGlobs.LoadScreen = Image_LoadBMPScaled(loadScreenName, 0, 0);
    loaderGlobs.ShutdownScreen = Image_LoadBMPScaled(shutdownScreenName, 0, 0);
    loaderGlobs.font = font5HI;
    loaderGlobs.currentSection = NULL;
    loaderGlobs.progressBar = Image_LoadBMPScaled(progressBarName, 0, 0);
    loaderGlobs.progressWindow = *progressWindow;
    loaderGlobs.progressDirection = progressDirection;

    if (loadingText) {
        loaderGlobs.loadingText = string_duplicate(loadingText);
        for (s = loaderGlobs.loadingText; *s != '\0'; s++) {
            if (*s == '_') *s = ' ';
        }
        formatted_text_printf(font5HI, &loaderGlobs.loadingWidth, NULL, "%s", loadingText);
    } else {
        loaderGlobs.loadingText = NULL;
    }

    if (profileName) file = file_open(profileName, "r");
    else file = NULL;

    for (loop = 0; loop < LOADER_MAXSECTIONS; loop++) {
        if (file && file_gets(line, sizeof(line), file) &&
            // STRING: LEGORR 0x004a52ac
            sound_variant_split(line, argv, "=") == 2) {
            loaderGlobs.sectionList[loop].name = (char*)malloc(strlen(argv[0]) + 1);
            strcpy(loaderGlobs.sectionList[loop].name, argv[0]);
            loaderGlobs.sectionList[loop].totalSize = atoi(argv[1]);
            loaderGlobs.sectionList[loop].currentSize = 0;
        } else {
            loaderGlobs.sectionList[loop].name = NULL;
        }
    }

    if (file) file_close(file);

    file_set_load_callback(loader_file_load_callback, NULL);
}

// FUNCTION: LEGORR 0x0044dd40
void loader_shutdown(const char* profileName)
{
    File* file;
    unsigned int loop;
    unsigned int size;

    if (profileName) file = file_open(profileName, "w");
    else file = NULL;

    for (loop = 0; loop < LOADER_MAXSECTIONS; loop++) {
        if (loaderGlobs.sectionList[loop].name) {
            size = loaderGlobs.sectionList[loop].currentSize;
            if (size == 0) size = loaderGlobs.sectionList[loop].totalSize;
            // STRING: LEGORR 0x004a52b0
            if (file) file_printf(file, "%s%s%i\n", loaderGlobs.sectionList[loop].name, "=", size);
            free(loaderGlobs.sectionList[loop].name);
        }
    }

    if (file) file_close(file);

    Image_Remove(loaderGlobs.LoadScreen);
    if (loaderGlobs.ShutdownScreen) Image_Remove(loaderGlobs.ShutdownScreen);
}

// FUNCTION: LEGORR 0x0044dde0
void loader_display_loading_bar(const char* section)
{
    LoaderSection* emptySection;
    unsigned int loop;

    if (section) {
        emptySection = NULL;
        for (loop = 0; loop < LOADER_MAXSECTIONS; loop++) {
            if (loaderGlobs.sectionList[loop].name) {
                if (_strcmpi(loaderGlobs.sectionList[loop].name, section) == 0) {
                    loaderGlobs.currentSection = &loaderGlobs.sectionList[loop];
                    loaderGlobs.currentSection->currentSize = 0;
                    break;
                }
            } else {
                emptySection = &loaderGlobs.sectionList[loop];
            }
        }

        if (loop == LOADER_MAXSECTIONS && emptySection) {
            emptySection->name = (char*)malloc(strlen(section) + 1);
            strcpy(emptySection->name, section);
            emptySection->currentSize = 0;
            emptySection->totalSize = 0;
            loaderGlobs.currentSection = emptySection;
        }

        loaderGlobs.progressLast = -1.0f;
        loaderGlobs.flags |= LOADER_GLOB_FLAG_ENABLED;
        loader_file_load_callback(NULL, 0, NULL);
    } else {
        loaderGlobs.flags &= ~LOADER_GLOB_FLAG_ENABLED;
    }
}

// FUNCTION: LEGORR 0x0044dec0
void loader_file_load_callback(const char* filename, unsigned int fileSize, void* data)
{
    float percent;

    if (loaderGlobs.flags & LOADER_GLOB_FLAG_ENABLED) {
        if (loaderGlobs.currentSection) loaderGlobs.currentSection->currentSize += fileSize;

        if (loaderGlobs.currentSection && loaderGlobs.currentSection->totalSize) {
            percent = (float)loaderGlobs.currentSection->currentSize /
                      (float)loaderGlobs.currentSection->totalSize;
        } else {
            percent = 0.0f;
        }

        if (percent > (loaderGlobs.progressLast - -0.002f)) {
            Point2F destPos;
            Point2F destSize;
            float newPos;

            destPos.x = loaderGlobs.progressWindow.x;
            destPos.y = loaderGlobs.progressWindow.y;
            destSize.x = loaderGlobs.progressWindow.width;
            destSize.y = loaderGlobs.progressWindow.height;

            Image_DisplayScaled(loaderGlobs.LoadScreen, NULL, NULL, NULL);

            if (percent > 1.0f) percent = 1.0f;

            switch (loaderGlobs.progressDirection) {
            case DIRECTION_UP:
                newPos = (destSize.y + destPos.y) - (destSize.y * percent);
                destSize.y = (destSize.y + destPos.y) - newPos;
                destPos.y = newPos;
                break;
            case DIRECTION_RIGHT:
                destSize.x *= percent;
                break;
            case DIRECTION_DOWN:
                destSize.y *= percent;
                break;
            case DIRECTION_LEFT:
                newPos = (destSize.x + destPos.x) - (destSize.x * percent);
                destSize.x = (destSize.x + destPos.x) - newPos;
                destPos.x = newPos;
                break;
            }

            if (loaderGlobs.progressBar) {
                Image_DisplayScaled(loaderGlobs.progressBar, NULL, &destPos, &destSize);
            }

            if (loaderGlobs.loadingText) {
                font_print_f(loaderGlobs.font,
                             (int)(loaderGlobs.progressWindow.x + (loaderGlobs.progressWindow.width / 2)) -
                                 (loaderGlobs.loadingWidth / 2),
                             (int)loaderGlobs.progressWindow.y - 1, "%s", loaderGlobs.loadingText);
            }

            main_loop_frame_pump(0);
            loaderGlobs.progressLast = percent;
        }
    }
}

// FUNCTION: LEGORR 0x0044e0a0
void loader_display_shutdown(void)
{
    if (loaderGlobs.ShutdownScreen) {
        Image_DisplayScaled(loaderGlobs.ShutdownScreen, NULL, NULL, NULL);
        directdraw_flip();
    }
}

#endif /* !LEGORRR_BACKEND_MINIWIN */
