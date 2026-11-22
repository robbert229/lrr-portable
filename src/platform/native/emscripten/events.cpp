// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web page's game events (events.h). Adapted from isle-portable's
 * ISLE/emscripten/events.cpp (LGPL-3.0). */
#include "platform/native/emscripten/events.h"

#include <SDL3/SDL_stdinc.h>
#include <emscripten.h>

#include <string>

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

/* `p_text` as a JSON string's contents. */
static std::string Emscripten_JsonEscape(const char* p_text)
{
    std::string text;
    for (const char* c = p_text; *c; c++) {
        if (*c == '"' || *c == '\\') {
            text += '\\';
            text += *c;
        }
        else if ((unsigned char) *c < 0x20) {
            char escape[8];
            SDL_snprintf(escape, sizeof(escape), "\\u%04x", (unsigned) (unsigned char) *c);
            text += escape;
        }
        else {
            text += *c;
        }
    }
    return text;
}

void Emscripten_SendDataProgress(const char* p_name, int p_progress)
{
    /* The name is a path from the host's manifest. */
    std::string json = "{\"name\": \"" + Emscripten_JsonEscape(p_name) + "\", \"progress\": " + std::to_string(p_progress) + "}";
    Emscripten_SendEvent("dataProgress", json.c_str());
}

void Emscripten_SendDataError(const char* p_message)
{
    std::string json = "{\"message\": \"" + Emscripten_JsonEscape(p_message) + "\"}";
    Emscripten_SendEvent("dataError", json.c_str());
}

void Emscripten_SendDataReady()
{
    Emscripten_SendEvent("dataReady", NULL);
}

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
