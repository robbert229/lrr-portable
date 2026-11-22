// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web page's game events (events.h). Adapted from isle-portable's
 * ISLE/emscripten/events.cpp (LGPL-3.0). */
#include "platform/native/emscripten/events.h"

#include <SDL3/SDL_stdinc.h>
#include <emscripten.h>

// clang-format off
static void Emscripten_SendEvent(const char* p_event, const char* p_json)
{
    MAIN_THREAD_EM_ASM({
        const eventName = UTF8ToString($0);
        let eventDetail = {};

        if ($1 && UTF8ToString($1).length > 0) {
            eventDetail = JSON.parse(UTF8ToString($1));
        }

        const targetElement = Module.canvas || window;
        const event = new CustomEvent(eventName, {detail : eventDetail});
        targetElement.dispatchEvent(event);
    }, p_event, p_json);
}
// clang-format on

void Emscripten_SendSaveSlotWritten(int p_slot)
{
    char buf[32];
    SDL_snprintf(buf, sizeof(buf), "{\"slot\": %d}", p_slot);
    Emscripten_SendEvent("saveSlotWritten", buf);
}

void Emscripten_SendSaveStateChanged()
{
    Emscripten_SendEvent("saveStateChanged", NULL);
}
