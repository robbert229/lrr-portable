// SPDX-License-Identifier: LGPL-3.0-or-later
/* The native process entry (ADR 0007): builds the Win32 command line from
 * argv and calls the units' WinMain (gods98/main.c), as the Windows loader
 * does for the shipped LegoRR.exe. miniwin supplies the window and the
 * message pump over SDL3.
 *
 * The command line starts with the program name, as GetCommandLineA returns
 * it: WinMain opens <name>0.wad, <name>1.wad, ... in the working directory,
 * so the name is always the original's "LegoRR.exe", whatever this
 * executable is called. The rest goes to WinMain as lpCmdLine, so the
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
    /* WinMain takes the WAD prefix (LegoRR0.wad, LegoRR1.wad) and the
     * Lego.cfg root token from the program's basename, as the shipped
     * LegoRR.exe was named. The port's executable is lrr-portable, so it
     * always passes the original's name in place of argv[0]. It is quoted
     * and has a directory, as the Windows shell starts a program: WinMain
     * takes the name after the last backslash and ends it at the first
     * quote, so a bare "LegoRR.exe" would leave it an empty name. */
    g_commandLine = "\".\\LegoRR.exe\"";
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
