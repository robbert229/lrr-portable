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
 * option does not reach WinMain. */
#include <SDL3/SDL.h>

#include <miniwin/windows.h>

#include "platform/native/config.h"

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
        if (strcmp(argv[i], "--ini") == 0 && i + 1 < argc) {
            iniPath = argv[++i];
            continue;
        }
        if (!arguments.empty()) arguments += ' ';
        arguments += QuoteArgument(argv[i]);
    }
#if defined(__EMSCRIPTEN__) || defined(__3DS__)
    /* The web build's argv[0] is "./this.program", and the 3DS's is the
     * .3dsx's path (sdmc:/3ds/LegoRR.3dsx, under any name the user gave
     * it) or nothing at all for a .cia; WinMain names the WADs after the
     * program, so it gets a relative LegoRR, as a native run from its game
     * directory does. */
    std::string program = "./LegoRR";
#else
    std::string program = argc > 0 ? argv[0] : "LegoRR.exe";
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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError());
        return 1;
    }
    if (!LRR_LoadConfig(iniPath)) {
        SDL_Quit();
        return 1;
    }
    int result = WinMain(NULL, NULL, const_cast<LPSTR>(arguments.c_str()), SW_SHOWNORMAL);
    SDL_Quit();
    return result;
}
