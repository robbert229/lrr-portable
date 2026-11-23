// SPDX-License-Identifier: LGPL-3.0-or-later
/* The native process entry (ADR 0007): builds the Win32 command line from
 * argv and calls the units' WinMain (gods98/main.c), as the Windows loader
 * does for the shipped LegoRR.exe. miniwin supplies the window and the
 * message pump over SDL3.
 *
 * The command line keeps the program name first, as GetCommandLineA returns
 * it, in Windows form (quoted, with backslash separators): WinMain takes the program
 * name after the last backslash and opens <name>0.wad, <name>1.wad, ... in
 * the working directory. The rest goes to WinMain as lpCmdLine, so the
 * shipped exe's flags (-best, -window, -nosound, -debug, ...) work
 * unchanged.
 *
 * Before WinMain it reads the port's configuration file (config.h), as
 * isle-portable's IsleApp does before it starts the game: --ini <path>
 * names the file, which otherwise lives in the preference directory. The
 * option does not reach WinMain.
 *
 * --version prints the port's version (lrr_version.h, written by
 * tools/version.cmake; docs/releasing.md) and exits; every run also logs it
 * at start-up. */
#include <SDL3/SDL.h>
#ifdef __ANDROID__
/* SDL's activity calls the program's SDL_main: SDL_main.h renames main
 * and exports it from libLegoRR.so. */
#include <SDL3/SDL_main.h>
#endif

#include <miniwin/windows.h>
#include <miniwin/miniwindevice.h>

#include "platform/native/config.h"

#include "lrr_version.h"

#include <cstdio>
#include <cstring>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int cmdShow);

/* GetCommandLineA's string (seam_stubs.cpp). */
extern "C" char* seam_command_line;

namespace {

std::string g_commandLine;

/* Quotes an argument that holds a blank, as the Windows shell does. */
std::string QuoteArgument(const char* arg)
{
    std::string text(arg);
    if (text.find_first_of(" \t") == std::string::npos) return text;
    return "\"" + text + "\"";
}

} // namespace

int main(int argc, char** argv)
{
    std::string arguments;
    const char* iniPath = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("lrr-portable %s\n", LRR_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--ini") == 0 && i + 1 < argc) {
            iniPath = argv[++i];
            continue;
        }
        if (!arguments.empty()) arguments += ' ';
        arguments += QuoteArgument(argv[i]);
    }
#if defined(__EMSCRIPTEN__) || defined(__3DS__) || defined(__ANDROID__)
    /* The web build's argv[0] is "./this.program", Android's is
     * "app_process", and the 3DS's is the .3dsx's path
     * (sdmc:/3ds/LegoRR.3dsx, under any name the user gave it) or nothing at
     * all for a .cia; WinMain names the WADs after the program, so it gets a
     * relative LegoRR, as a native run from its game directory does. */
    std::string program = "./LegoRR";
#else
    std::string program = argc > 0 ? argv[0] : "LegoRR.exe";
    /* A program started from PATH (the Flatpak's command, or LegoRR on an
     * installed system) has a bare argv[0]. Windows always puts a path in
     * the command line, and WinMain relies on it: it takes the name after
     * the last backslash, so without one it keeps the opening quote and
     * then cuts the name at it, leaving no name and no WADs. */
    if (program.find('/') == std::string::npos) program = "./" + program;
#endif
    for (char& c : program) {
        if (c == '/') c = '\\';
    }
    /* Quoted, as the Windows shell starts a program: WinMain ends the
     * program name at the first quote. */
    g_commandLine = "\"" + program + "\"";
    if (!arguments.empty()) g_commandLine += " " + arguments;
    seam_command_line = &g_commandLine[0];

#ifdef __EMSCRIPTEN__
    /* WinMain blocks on a worker (-sPROXY_TO_PTHREAD). A WebGL canvas
     * shows a frame only when its thread returns to the browser, which
     * WinMain never does, so SDL renders in software and presents each
     * frame on the page's canvas from the main browser thread. */
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, SDL_SOFTWARE_RENDERER);
#endif
#ifdef __ANDROID__
    /* The game is landscape. SDL lets a resizable window take any
     * orientation, and the activity's own landscape setting with it. */
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
    SDL_SetAppMetadata("LRR Portable", LRR_VERSION, NULL);
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("lrr-portable %s", LRR_VERSION);
    if (!LRR_LoadConfig(iniPath)) {
        SDL_Quit();
        return 1;
    }
    /* WinMain creates its window before it lists the 3D devices, and
     * miniwin lists its OpenGL devices only on an OpenGL window. */
    const char* device = LRR_GetDisplayConfig()->device;
    if (LRR_UseOpenGLWindow(device, LRR_Default3DDevice())) {
        SDL_Log("The 3D device is expected to be an OpenGL device: creating an OpenGL window");
        Miniwin_SetOpenGLWindows(true);
    }
    int result = WinMain(NULL, NULL, const_cast<LPSTR>(arguments.c_str()), SW_SHOWNORMAL);
    SDL_Quit();
    return result;
}
